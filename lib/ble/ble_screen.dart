// lib/ble/ble_screen.dart
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;
import 'package:shared_preferences/shared_preferences.dart';

import '../di.dart';
import 'ble_repository.dart';
import '../widgets/app_scaffold.dart';
import '../widgets/status_badge.dart';
import '../test_ids.dart';
import 'ble_foreground_service.dart';
import 'ble_permissions.dart';

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});

  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  late final BleRepository repo;

  StreamSubscription<DiscoveredDevice>? _scanStreamSub;
  StreamSubscription<String>? _pairingSub;
  StreamSubscription<String>? _consoleSub;

  final List<DiscoveredDevice> _devices = [];
  final List<String> _consoleLines = [];
  final TextEditingController _sendCtrl = TextEditingController();

  // ✅ Uložené zariadenie (z minulého pripojenia)
  String? _savedDeviceId;
  String? _savedDeviceName;

  @override
  void initState() {
    super.initState();

    repo = getIt<BleRepository>();

    // Scan results - filter iba naše hodinky
    _scanStreamSub = repo.scannedDevices.listen((d) {
      if (!mounted) return;

      // ✅ Filter podľa mena hodiniek
      if (!d.name.toLowerCase().contains('smartwatch')) return;

      final i = _devices.indexWhere((x) => x.id == d.id);
      setState(() {
        if (i == -1) {
          _devices.add(d);
        } else {
          _devices[i] = d;
        }
      });
    });

    // Console history
    _consoleLines
      ..clear()
      ..addAll(repo.consoleHistory);

    _consoleSub = repo.console.listen((line) {
      if (!mounted) return;
      setState(() {
        _consoleLines.insert(0, line);
        if (_consoleLines.length > 500) _consoleLines.removeLast();
      });
    });

    // Pairing PIN dialóg
    _pairingSub = repo.pairingPins.listen((pin) async {
      if (!mounted) return;

      final accepted = await _showPairingDialogOkCancel(pin);
      if (accepted) {
        await repo.confirmPairing();
        await startBleService();
      } else {
        await repo.rejectPairing();
        await stopBleService();
      }
    });

    // Pri štarte načítaj uložené zariadenie (nepripája sa automaticky)
    _loadSavedDevice();

    // Spusti scan IBA ak nie sme connected
    if (repo.currentStatus != BleStatus.connected) {
      _initScan();
    }
  }

  /// Načíta uložené zariadenie z minulého pripojenia
  Future<void> _loadSavedDevice() async {
    final prefs = await SharedPreferences.getInstance();
    final lastId = prefs.getString(BleRepository.kWatchId);
    final lastName = prefs.getString('ble_last_device_name');

    if (lastId != null && lastId.isNotEmpty) {
      setState(() {
        _savedDeviceId = lastId;
        _savedDeviceName = lastName ?? 'SmartWatch';
      });
      debugPrint('Loaded saved device: $lastId ($lastName)');
    }
  }

  /// Uloží zariadenie po úspešnom pripojení
  Future<void> _saveDevice(String id, String name) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(BleRepository.kWatchId, id);
    await prefs.setString('ble_last_device_name', name);
    setState(() {
      _savedDeviceId = id;
      _savedDeviceName = name;
    });
  }

  Future<void> _initScan() async {
    final granted = await ensureBlePermissions();
    if (!mounted) return;

    if (granted) {
      repo.startScan(timeout: const Duration(seconds: 20), filterService: false);
    } else {
      _showPermissionDeniedSnackbar();
    }
  }

  Future<void> _startScanWithPermissions() async {
    final granted = await ensureBlePermissions();
    if (!mounted) return;

    if (!granted) {
      _showPermissionDeniedSnackbar();
      return;
    }

    _devices.clear();
    repo.startScan(
      timeout: const Duration(seconds: 20),
      filterService: false,
    );
    setState(() {});
  }

  void _showPermissionDeniedSnackbar() {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('BLE/Location povolenia neboli udelené. Povoľte ich v nastaveniach.'),
        duration: Duration(seconds: 4),
      ),
    );
  }

  @override
  void dispose() {
    _scanStreamSub?.cancel();
    _pairingSub?.cancel();
    _consoleSub?.cancel();
    _sendCtrl.dispose();
    super.dispose();
  }

  Future<bool> _showPairingDialogOkCancel(String pin) async {
    final res = await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => Dialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        child: Padding(
          padding: const EdgeInsets.all(18),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Text(
                'PIN',
                style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
              ),
              const SizedBox(height: 10),
              SelectableText(
                pin,
                style: const TextStyle(
                  fontSize: 40,
                  fontWeight: FontWeight.w900,
                  letterSpacing: 4,
                ),
              ),
              const SizedBox(height: 18),
              Row(
                children: [
                  Expanded(
                    child: OutlinedButton(
                      onPressed: () => Navigator.of(ctx).pop(false),
                      child: const Text('Zrušiť'),
                    ),
                  ),
                  const SizedBox(width: 10),
                  Expanded(
                    child: ElevatedButton(
                      onPressed: () => Navigator.of(ctx).pop(true),
                      child: const Text('OK'),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );

    return res == true;
  }

  @override
  Widget build(BuildContext context) {
    // ✅ StreamBuilder pre status - vždy aktuálny!
    return StreamBuilder<BleStatus>(
      stream: repo.status,
      initialData: repo.currentStatus,
      builder: (context, statusSnapshot) {
        final status = statusSnapshot.data ?? BleStatus.idle;

        // ✅ StreamBuilder pre pairing status
        return StreamBuilder<bool>(
          stream: repo.pairedStatus,
          initialData: repo.isPaired,
          builder: (context, pairedSnapshot) {
            final isPaired = pairedSnapshot.data ?? false;

            return _buildScreen(context, status, isPaired);
          },
        );
      },
    );
  }

  Widget _buildScreen(BuildContext context, BleStatus status, bool isPaired) {
    // Status badge
    Widget badge;
    switch (status) {
      case BleStatus.connecting:
        badge = const StatusBadge.connecting();
        break;
      case BleStatus.connected:
        badge = const StatusBadge.connected();
        break;
      case BleStatus.scanning:
        badge = const StatusBadge.custom(text: 'Skenujem...', color: Colors.blue);
        break;
      default:
        badge = const StatusBadge.disconnected();
        break;
    }

    final pairingChip = Chip(
      label: Text(isPaired ? 'Spárované' : 'Nespárované'),
      avatar: Icon(isPaired ? Icons.verified : Icons.link_off, size: 18),
    );

    return AppScaffold(
      title: 'BLE hodinky',
      titleKey: TKeys.titleBle,
      actions: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8),
          child: badge,
        ),
      ],
      body: Column(
        children: [
          const SizedBox(height: 8),

          // Tlačidlá
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ElevatedButton.icon(
                key: TKeys.bleBtnSearch,
                onPressed: status == BleStatus.scanning ? null : _startScanWithPermissions,
                icon: status == BleStatus.scanning
                    ? const SizedBox(
                  width: 16,
                  height: 16,
                  child: CircularProgressIndicator(strokeWidth: 2),
                )
                    : const Icon(Icons.search),
                label: Text(status == BleStatus.scanning ? 'Hľadám...' : 'Hľadať'),
              ),
              const SizedBox(width: 12),
              OutlinedButton.icon(
                key: TKeys.bleBtnStop,
                onPressed: repo.stopScan,
                icon: const Icon(Icons.stop),
                label: const Text('Stop'),
              ),
              const SizedBox(width: 12),
              OutlinedButton.icon(
                onPressed: status == BleStatus.connected ? repo.disconnect : null,
                icon: const Icon(Icons.link_off),
                label: const Text('Odpojiť'),
              ),
            ],
          ),

          const SizedBox(height: 8),

          // Pairing info
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              pairingChip,
              const SizedBox(width: 10),
              Text(
                'ID: ${BleRepository.kWatchId}',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          ),

          const Divider(),

          // Zoznam zariadení
          Expanded(
            flex: 4,
            child: _buildDeviceList(status, isPaired),
          ),

          const Divider(),

          // TODO: Konzola je zakomentovaná
          // const Divider(),
          // Expanded(
          //   flex: 3,
          //   child: _buildConsole(context),
          // ),
          // _buildInputBar(status),
        ],
      ),
    );
  }

  /// Zoznam zariadení - uložené + naskenované + connected
  Widget _buildDeviceList(BleStatus status, bool isPaired) {
    // Zozbieraj všetky zariadenia na zobrazenie
    final List<_DeviceItem> items = [];

    // ID aktuálne pripojeného zariadenia
    final connectedId = repo.deviceId;

    // 1. Ak sme connected, pridaj pripojené zariadenie ako prvé
    if (status == BleStatus.connected && connectedId != null) {
      final inScanned = _devices.any((d) => d.id == connectedId);
      if (!inScanned) {
        // Pripojené zariadenie nie je v scane - pridaj ho
        items.add(_DeviceItem(
          id: connectedId,
          name: _savedDeviceName ?? 'SmartWatch',
          rssi: null,
          isSaved: connectedId == _savedDeviceId,
          isConnected: true,
        ));
      }
    }

    // 2. Pridaj uložené zariadenie (ak nie je už pridané a nie je v _devices)
    if (_savedDeviceId != null && _savedDeviceId != connectedId) {
      final inScanned = _devices.any((d) => d.id == _savedDeviceId);
      if (!inScanned) {
        items.add(_DeviceItem(
          id: _savedDeviceId!,
          name: _savedDeviceName ?? 'SmartWatch',
          rssi: null, // nie je v dosahu
          isSaved: true,
        ));
      }
    }

    // 3. Pridaj naskenované zariadenia
    for (final d in _devices) {
      items.add(_DeviceItem(
        id: d.id,
        name: d.name.isEmpty ? 'SmartWatch' : d.name,
        rssi: d.rssi,
        isSaved: d.id == _savedDeviceId,
        isConnected: status == BleStatus.connected && d.id == connectedId,
      ));
    }

    // Prázdny stav
    if (items.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('Žiadne zariadenia.', key: TKeys.bleEmptyText),
            if (status == BleStatus.scanning) ...[
              const SizedBox(height: 16),
              const CircularProgressIndicator(),
            ],
          ],
        ),
      );
    }

    return ListView.builder(
      itemCount: items.length,
      itemBuilder: (context, i) {
        final item = items[i];
        final isConnected = item.isConnected ||
            (status == BleStatus.connected && repo.deviceId == item.id);
        final isConnecting = status == BleStatus.connecting && repo.deviceId == item.id;

        return ListTile(
          leading: Icon(
            Icons.watch,
            color: isConnected
                ? Colors.green
                : item.isSaved
                ? Colors.blue
                : null,
          ),
          title: Row(
            children: [
              Text(item.name),
              if (item.isSaved) ...[
                const SizedBox(width: 8),
                const Icon(Icons.star, size: 16, color: Colors.amber),
              ],
            ],
          ),
          subtitle: Text(
            isConnected
                ? 'Pripojené'
                : item.rssi != null
                ? 'RSSI: ${item.rssi} dBm'
                : 'Uložené zariadenie (nie je v dosahu)',
          ),
          trailing: isConnected
              ? const Chip(
            label: Text('Pripojené'),
            backgroundColor: Colors.green,
          )
              : isConnecting
              ? const SizedBox(
            width: 24,
            height: 24,
            child: CircularProgressIndicator(strokeWidth: 2),
          )
              : item.isSaved
              ? const Chip(label: Text('Uložené'))
              : const Icon(Icons.chevron_right),
          onTap: (isConnected || isConnecting)
              ? null
              : () {
            // Pripoj sa a ulož zariadenie
            repo.connect(item.id);
            _saveDevice(item.id, item.name);
          },
        );
      },
    );
  }

  Widget _buildConsole(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 12),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(12),
        color: Theme.of(context).colorScheme.surfaceVariant.withOpacity(0.25),
        border: Border.all(
          color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
        ),
      ),
      child: _consoleLines.isEmpty
          ? const Align(
        alignment: Alignment.topLeft,
        child: Text('Konzola je prázdna.'),
      )
          : ListView.separated(
        reverse: true,
        itemCount: _consoleLines.length,
        separatorBuilder: (_, __) => const Divider(height: 12),
        itemBuilder: (_, i) => Text(_consoleLines[i]),
      ),
    );
  }

  Widget _buildInputBar(BleStatus status) {
    final canSend = status == BleStatus.connected;

    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _sendCtrl,
              enabled: canSend,
              decoration: InputDecoration(
                hintText: canSend ? 'Správa pre hodinky…' : 'Najprv sa pripoj...',
                border: const OutlineInputBorder(),
                isDense: true,
              ),
              onSubmitted: canSend ? _send : null,
            ),
          ),
          const SizedBox(width: 8),
          ElevatedButton.icon(
            onPressed: canSend ? () => _send(_sendCtrl.text) : null,
            icon: const Icon(Icons.send),
            label: const Text('Poslať'),
          ),
        ],
      ),
    );
  }

  void _send(String text) {
    final t = text.trim();
    if (t.isEmpty) return;
    _sendCtrl.clear();
    repo.sendString(t);
  }
}

/// Helper trieda pre zobrazenie zariadenia v liste
class _DeviceItem {
  final String id;
  final String name;
  final int? rssi;
  final bool isSaved;
  final bool isConnected;

  _DeviceItem({
    required this.id,
    required this.name,
    this.rssi,
    this.isSaved = false,
    this.isConnected = false,
  });
}