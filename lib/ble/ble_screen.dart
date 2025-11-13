// lib/ble/lib/ble_screen.dart

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import 'ble_permissions.dart'; // je v tom istom priečinku
import '../../widgets/app_scaffold.dart';
import '../../widgets/status_badge.dart';

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});

  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  final _ble = FlutterReactiveBle();
  StreamSubscription<DiscoveredDevice>? _scanSub;

  final List<DiscoveredDevice> _devices = [];
  bool _isScanning = false;

  @override
  void initState() {
    super.initState();
    _startScan();
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    super.dispose();
  }

  Future<void> _startScan() async {
    // 1) permissions
    final ok = await ensureBlePermissions();
    if (!ok) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Bez povolení neviem hľadať BLE zariadenia.'),
          ),
        );
      }
      return;
    }

    // 2) počkaj, kým je BLE v stave ready
    final status = await _ble.statusStream.first;
    if (status != BleStatus.ready) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Bluetooth nie je pripravený: $status')),
        );
      }
      return;
    }

    // 3) spusti scan bez filtrov na služby
    setState(() {
      _isScanning = true;
      _devices.clear();
    });

    _scanSub?.cancel();

    _scanSub = _ble
        .scanForDevices(
      withServices:
      const [], // DÔLEŽITÉ: žiadne UUID filtre – nech vidíme všetko
      scanMode: ScanMode.lowLatency,
    )
        .listen(
          (device) {
        // väčšina ESP/NimBLE projektov používa zmysluplné meno
        if (device.name.isEmpty) return;

        // TODO: keď budete vedieť presný názov hodiniek,
        // uprav si tento filter, napr.:
        // final isWatch = device.name.toLowerCase().startsWith('c6-watch');
        final lowerName = device.name.toLowerCase();
        final isWatch = lowerName.contains('watch') ||
            lowerName.contains('esp') ||
            lowerName.contains('smart');

        if (!isWatch) {
          // ak chceš, môžeš túto podmienku úplne vyhodiť
          // aby si videl úplne všetky zariadenia
          return;
        }

        final index = _devices.indexWhere((d) => d.id == device.id);
        setState(() {
          if (index == -1) {
            _devices.add(device);
          } else {
            _devices[index] = device;
          }
        });
      },
      onError: (e) {
        if (!mounted) return;
        setState(() => _isScanning = false);
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Chyba pri scane: $e')),
        );
      },
      onDone: () {
        if (!mounted) return;
        setState(() => _isScanning = false);
      },
    );
  }

  void _stopScan() {
    _scanSub?.cancel();
    setState(() => _isScanning = false);
  }

  @override
  Widget build(BuildContext context) {
    return AppScaffold(
      title: 'BLE hodinky',
      actions: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8),
          child: _isScanning
              ? const StatusBadge.connecting()
              : const StatusBadge.disconnected(),
        ),
      ],

      body: Column(
        children: [
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ElevatedButton.icon(
                onPressed: _isScanning ? null : _startScan,
                icon: const Icon(Icons.search),
                label: const Text('Hľadať'),
              ),
              const SizedBox(width: 12),
              OutlinedButton.icon(
                onPressed: _isScanning ? _stopScan : null,
                icon: const Icon(Icons.stop),
                label: const Text('Stop'),
              ),
            ],
          ),
          const Divider(),
          Expanded(
            child: _devices.isEmpty
                ? const Center(
              child: Text(
                'Nenašli sa žiadne hodinky.\n'
                    'Skontroluj, že sú zapnuté a v blízkosti\n'
                    'a že vysielajú BLE (advertising).',
                textAlign: TextAlign.center,
              ),
            )
                : ListView.builder(
              itemCount: _devices.length,
              itemBuilder: (context, index) {
                final d = _devices[index];
                return ListTile(
                  leading: const Icon(Icons.watch),
                  title: Text(d.name),
                  subtitle: Text('ID: ${d.id}\nRSSI: ${d.rssi} dBm'),
                  isThreeLine: true,
                  onTap: () {
                    // tu neskôr spravíme connectToDevice(d.id)
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        content: Text('Klikol si na: ${d.name}'),
                      ),
                    );
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
