// lib/ble/lib/ble_screen.dart
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;

import '../../di.dart';
import '../../ble/ble_repository.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/status_badge.dart';
import '../test_ids.dart';
import 'ble_foreground_service.dart';

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

  final List<DiscoveredDevice> _devices = [];
  BleStatus _status = BleStatus.idle;

  // Console + input
  final List<String> _consoleLines = [];
  final TextEditingController _sendCtrl = TextEditingController();

  @override
  void initState() {
    super.initState();

    // ✅ Repo zo singleton DI (GetIt) -> prežije odchod zo stránky
    repo = getIt<BleRepository>();

    // status
    _statusSub = repo.status.listen((s) async {
      if (!mounted) return;
      setState(() => _status = s);
      if (s == BleStatus.connected){
        await startBleService();
      }
    });

    // scan results
    _scanStreamSub = repo.scannedDevices.listen((d) {
      if (!mounted) return;
      final i = _devices.indexWhere((x) => x.id == d.id);
      setState(() {
        if (i == -1) _devices.add(d);
        else _devices[i] = d;
      });
    });

    // ✅ console history + live updates
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

    // pairing dialog
    _pairingSub = repo.pairingPins.listen((pin) async {
      if (!mounted) return;

      final accepted = await _showPairingDialogOkCancel(pin);
      if (accepted) {
        await repo.confirmPairing();
        // ✅ tu spusti foreground service
        await startBleService();
      } else {
        await repo.rejectPairing();
        await stopBleService();
      }
    });

    // scan len ak nie sme práve connected (aby sme neotravovali)
    if (_status != BleStatus.connected) {
      repo.startScan(timeout: const Duration(seconds: 20), filterService: false);
    }
  }

  @override
  void dispose() {
    _scanStreamSub?.cancel();
    _statusSub?.cancel();
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

          // Buttons
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ElevatedButton.icon(
                key: TKeys.bleBtnSearch,
                onPressed: () {
                  _devices.clear();
                  repo.startScan(
                    timeout: const Duration(seconds: 20),
                    filterService: false,
                  );
                  setState(() {});
                },
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
                onPressed: repo.disconnect, // manuálne odpojenie
                icon: const Icon(Icons.link_off),
                label: const Text('Odpojiť'),
              ),
            ],
          ),

          const Divider(),

          // Devices list
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
                  title: Text(d.name.isEmpty ? '(bez názvu)' : d.name),
                  subtitle: Text('ID: ${d.id}\nRSSI: ${d.rssi}'),
                  isThreeLine: true,
                  onTap: () => repo.connect(d.id),
                );
              },
            ),
          ),

          const Divider(),

          // Console
          Expanded(
            flex: 3,
            child: _buildConsole(context),
          ),

          // Input bar
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
