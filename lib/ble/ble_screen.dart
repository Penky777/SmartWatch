// lib/ble/ble_screen.dart
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;

import '../di.dart';
import 'ble_repository.dart';
import '../widgets/app_scaffold.dart';
import '../widgets/status_badge.dart';
import '../test_ids.dart';
import 'ble_foreground_service.dart';
import 'ble_permissions.dart'; // ✅ PRIDANÉ

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});

  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  late final BleRepository repo;

  StreamSubscription<DiscoveredDevice>? _scanStreamSub;
  StreamSubscription<BleStatus>? _statusSub;
  StreamSubscription<String>? _pairingSub;
  StreamSubscription<String>? _consoleSub;

  // ✅ paired status
  StreamSubscription<bool>? _pairedSub;
  bool _isPaired = false;

  final List<DiscoveredDevice> _devices = [];
  BleStatus _status = BleStatus.idle;

  final List<String> _consoleLines = [];
  final TextEditingController _sendCtrl = TextEditingController();

  @override
  void initState() {
    super.initState();

    repo = getIt<BleRepository>();
    _status = repo.currentStatus;
    _isPaired = repo.isPaired;

    // ✅ načítaj pairing flag pre hodinky hneď po otvorení obrazovky
    repo.loadPairedForWatch().then((v) {
      if (!mounted) return;
      setState(() => _isPaired = v);
    });

    // ✅ počúvaj zmeny pairing stavu (confirm/reject)
    _pairedSub = repo.pairedStatus.listen((v) {
      if (!mounted) return;
      setState(() => _isPaired = v);
    });

    _statusSub = repo.status.listen((s) async {
      if (!mounted) return;
      setState(() => _status = s);
      if (s == BleStatus.connected) {
        await startBleService();
      }
    });

    // scan results
    _scanStreamSub = repo.scannedDevices.listen((d) {
      if (!mounted) return;

      final i = _devices.indexWhere((x) => x.id == d.id);
      setState(() {
        if (i == -1) {
          _devices.add(d);
        } else {
          _devices[i] = d;
        }
      });
    });

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

    // ✅ OPRAVA: Spusti scan s kontrolou permissions
    if (_status != BleStatus.connected) {
      _initScan();
    }
  }

  // ✅ NOVÁ METÓDA: Kontrola permissions pred prvým scanom
  Future<void> _initScan() async {
    final granted = await ensureBlePermissions();
    if (!mounted) return;

    if (granted) {
      repo.startScan(timeout: const Duration(seconds: 20), filterService: false);
    } else {
      _showPermissionDeniedSnackbar();
    }
  }

  // ✅ NOVÁ METÓDA: Scan s kontrolou permissions (pre tlačidlo Hľadať)
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

  // ✅ NOVÁ METÓDA: Snackbar pre odmietnuté povolenia
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
    _statusSub?.cancel();
    _pairingSub?.cancel();
    _consoleSub?.cancel();
    _pairedSub?.cancel();
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
    Widget badge;
    switch (_status) {
      case BleStatus.connecting:
        badge = const StatusBadge.connecting();
        break;
      case BleStatus.connected:
        badge = const StatusBadge.connected();
        break;
      default:
        badge = const StatusBadge.disconnected();
        break;
    }

    final pairingChip = Chip(
      label: Text(_isPaired ? 'Spárované' : 'Nespárované'),
      avatar: Icon(_isPaired ? Icons.verified : Icons.link_off, size: 18),
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

          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ElevatedButton.icon(
                key: TKeys.bleBtnSearch,
                // ✅ OPRAVA: Volá metódu s kontrolou permissions
                onPressed: _startScanWithPermissions,
                icon: const Icon(Icons.search),
                label: const Text('Hľadať'),
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
                onPressed: repo.disconnect,
                icon: const Icon(Icons.link_off),
                label: const Text('Odpojiť'),
              ),
            ],
          ),

          const SizedBox(height: 8),

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

          Expanded(
            flex: 4,
            child: _devices.isEmpty
                ? const Center(
              child: Text('Žiadne zariadenia.', key: TKeys.bleEmptyText),
            )
                : ListView.builder(
              itemCount: _devices.length,
              itemBuilder: (context, i) {
                final d = _devices[i];
                return ListTile(
                  leading: const Icon(Icons.watch),
                  title: Text(d.name.isEmpty ? 'Naše hodinky' : d.name),
                  subtitle: Text('ID: ${d.id}\nRSSI: ${d.rssi}'),
                  isThreeLine: true,
                  trailing: _isPaired
                      ? const Chip(label: Text('Spárované'))
                      : const Chip(label: Text('Nespárované')),
                  onTap: () => repo.connect(d.id),
                );
              },
            ),
          ),

          const Divider(),

          Expanded(
            flex: 3,
            child: _buildConsole(context),
          ),

          _buildInputBar(),
        ],
      ),
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

  Widget _buildInputBar() {
    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _sendCtrl,
              decoration: const InputDecoration(
                hintText: 'Správa pre hodinky…',
                border: OutlineInputBorder(),
                isDense: true,
              ),
              onSubmitted: _send,
            ),
          ),
          const SizedBox(width: 8),
          ElevatedButton.icon(
            onPressed: () => _send(_sendCtrl.text),
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