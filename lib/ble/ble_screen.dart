// lib/ble/lib/ble_screen.dart
import 'dart:async';

import 'package:flutter/material.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/status_badge.dart';
import '../../ble/ble_repository.dart'; // uprav cestu podľa teba
import '../../ble/ble_client.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;

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

  final List<DiscoveredDevice> _devices = [];
  BleStatus _status = BleStatus.idle;

  @override
  void initState() {
    super.initState();

    // ak už repo injektuješ cez DI/provider, tak toto vyhoď a použij to z DI
    repo = BleRepository(BleClient(FlutterReactiveBle()));

    _statusSub = repo.status.listen((s) {
      if (!mounted) return;
      setState(() => _status = s);
    });

    _scanStreamSub = repo.scannedDevices.listen((d) {
      if (!mounted) return;
      final i = _devices.indexWhere((x) => x.id == d.id);
      setState(() {
        if (i == -1) _devices.add(d);
        else _devices[i] = d;
      });
    });

    _pairingSub = repo.pairingPins.listen((pin) async {
      if (!mounted) return;
      final accepted = await _showPairingDialog(pin);
      if (accepted) {
        await repo.confirmPairing();
      } else {
        await repo.rejectPairing();
      }
    });

    // scan (na debug dávam filterService=false; keď bude všetko sedieť, daj true)
    repo.startScan(timeout: const Duration(seconds: 20), filterService: false);
  }

  @override
  void dispose() {
    _scanStreamSub?.cancel();
    _statusSub?.cancel();
    _pairingSub?.cancel();
    repo.dispose();
    super.dispose();
  }

  Future<bool> _showPairingDialog(String pin) async {
    final res = await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => AlertDialog(
        title: const Text('Pairing'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('Sedí tento PIN na hodinkách?'),
            const SizedBox(height: 12),
            SelectableText(
              pin,
              style: const TextStyle(fontSize: 28, fontWeight: FontWeight.w700, letterSpacing: 2),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.of(ctx).pop(false), child: const Text('Nie')),
          ElevatedButton(onPressed: () => Navigator.of(ctx).pop(true), child: const Text('Áno')),
        ],
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
      case BleStatus.disconnected:
      case BleStatus.idle:
      case BleStatus.scanning:
      case BleStatus.error:
        badge = const StatusBadge.disconnected();
        break;
    }

    return AppScaffold(
      title: 'BLE hodinky',
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
                onPressed: () {
                  _devices.clear();
                  repo.startScan(timeout: const Duration(seconds: 20), filterService: false);
                  setState(() {});
                },
                icon: const Icon(Icons.search),
                label: const Text('Hľadať'),
              ),
              const SizedBox(width: 12),
              OutlinedButton.icon(
                onPressed: repo.stopScan,
                icon: const Icon(Icons.stop),
                label: const Text('Stop'),
              ),
            ],
          ),
          const Divider(),
          Expanded(
            child: _devices.isEmpty
                ? const Center(child: Text('Žiadne zariadenia.'))
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
        ],
      ),
    );
  }
}
