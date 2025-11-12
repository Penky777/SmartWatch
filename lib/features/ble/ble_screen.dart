import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;

import '../../ble/ble_client.dart';
import '../../ble/ble_permissions.dart';
import '../../ble/ble_repository.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/satus_badge.dart';
import '../../widgets/ble/device_tile.dart';

enum _V { idle, scanning, connecting, connected, disconnected, error }

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});
  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  final repo = BleRepository(BleClient());
  final _devices = <DiscoveredDevice>[];
  final _msgCtrl = TextEditingController();
  _V _view = _V.idle;

  @override
  void initState() {
    super.initState();
    // mapovanie interného statusu na UI stav
    repo.status.listen((s) {
      setState(() {
        switch (s) {
          case BleStatus.idle: _view = _V.idle; break;
          case BleStatus.scanning: _view = _V.scanning; break;
          case BleStatus.connecting: _view = _V.connecting; break;
          case BleStatus.connected: _view = _V.connected; break;
          case BleStatus.disconnected: _view = _V.disconnected; break;
          case BleStatus.error: _view = _V.error; break;
        }
      });
    });
    _startScan();
  }

  Future<void> _startScan() async {
    final ok = await ensureBlePermissions();
    if (!ok) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Bluetooth povolenia odmietnuté")),
        );
      }
      return;
    }
    _devices.clear();
    repo.startScan(timeout: const Duration(seconds: 10)).listen((d) {
      if (_devices.indexWhere((e) => e.id == d.id) == -1) {
        setState(() => _devices.add(d));
      }
    });
  }

  @override
  void dispose() {
    repo.dispose();
    _msgCtrl.dispose();
    super.dispose();
  }

  Widget _statusBadge() {
    switch (_view) {
      case _V.connected: return const StatusBadge.connected();
      case _V.connecting: return const StatusBadge.connecting();
      case _V.disconnected: return const StatusBadge.disconnected();
      case _V.scanning: return const StatusBadge.custom(text: "Skenujem…", color: Colors.blue);
      case _V.error: return const StatusBadge.custom(text: "Chyba", color: Colors.redAccent);
      case _V.idle: default: return const StatusBadge.custom(text: "Pripravené", color: Colors.grey);
    }
  }

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;

    return AppScaffold(
      index: 1,
      appBar: AppBar(
        title: const Text("Hodinky • Bluetooth"),
        actions: [
          IconButton(
            tooltip: "Skenovať",
            onPressed: _startScan,
            icon: const Icon(Icons.bluetooth_searching),
          ),
          const SizedBox(width: 6),
        ],
      ),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 14, 16, 8),
            child: Row(
              children: [
                _statusBadge(),
                const Spacer(),
                if (_view == _V.connected && repo.deviceId != null)
                  Row(
                    children: [
                      Icon(Icons.watch, size: 18, color: cs.primary),
                      const SizedBox(width: 6),
                      Text(
                        repo.deviceId!.length > 8
                            ? "…${repo.deviceId!.substring(repo.deviceId!.length - 8)}"
                            : repo.deviceId!,
                        style: const TextStyle(fontWeight: FontWeight.w600),
                      ),
                    ],
                  ),
              ],
            ),
          ),

          // Zoznam zariadení
          Expanded(
            child: _devices.isEmpty
                ? Center(
              child: Opacity(
                opacity: 0.7,
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: const [
                    Icon(Icons.devices_other, size: 48),
                    SizedBox(height: 10),
                    Text("Žiadne zariadenia (skús obnoviť skenovanie)"),
                  ],
                ),
              ),
            )
                : ListView.builder(
              itemCount: _devices.length,
              itemBuilder: (_, i) {
                final d = _devices[i];
                return DeviceTile(
                  title: d.name,
                  subtitle: d.id,
                  rssi: d.rssi,
                  onTap: () async {
                    await repo.stopScan();
                    await repo.connect(d.id);
                    if (mounted) setState(() {});
                  },
                );
              },
            ),
          ),

          // Sekcia odoslania správy
          AnimatedSwitcher(
            duration: const Duration(milliseconds: 250),
            child: (_view == _V.connected && repo.deviceId != null)
                ? Padding(
              key: const ValueKey("send"),
              padding: const EdgeInsets.fromLTRB(16, 8, 16, 16),
              child: Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _msgCtrl,
                      textInputAction: TextInputAction.send,
                      onSubmitted: (_) async => _send(),
                      decoration: const InputDecoration(
                        labelText: "Správa → hodinky",
                        hintText: "napr. ping",
                      ),
                    ),
                  ),
                  const SizedBox(width: 10),
                  ElevatedButton.icon(
                    onPressed: _send,
                    icon: const Icon(Icons.send),
                    label: const Text("Poslať"),
                  ),
                ],
              ),
            )
                : const SizedBox.shrink(),
          ),
        ],
      ),
      child: const SizedBox.shrink(),
    );
  }

  Future<void> _send() async {
    final txt = _msgCtrl.text.trim();
    if (txt.isEmpty) return;
    try {
      await repo.sendString(txt);
      _msgCtrl.clear();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Odoslané")),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text("Nepodarilo sa odoslať: $e")),
        );
      }
    }
  }
}
