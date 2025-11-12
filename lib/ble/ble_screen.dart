// lib/features/ble/ble_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;
import 'ble_repository.dart';
import 'ble_client.dart';
import 'ble_permissions.dart';

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});
  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  final repo = BleRepository(BleClient());
  final _devices = <DiscoveredDevice>[];
  final _msgCtrl = TextEditingController();

  @override
  void initState() {
    super.initState();
    repo.status.listen((_) => setState(() {}));
    _startScan();
  }

  Future<void> _startScan() async {
    final ok = await ensureBlePermissions();
    if (!ok) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Bluetooth povolenia odmietnuté")));
      }
      return;
    }
    _devices.clear();
    repo.startScan(timeout: const Duration(seconds: 10)).listen((d) {
      // deduplikácia podľa id
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

  @override
  Widget build(BuildContext context) {
    final status = repo.status; // vnútorné, ale pre demo OK
    return Scaffold(
      appBar: AppBar(title: const Text("Hodinky – BLE párovanie")),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(12),
            child: Row(
              children: [
                ElevatedButton(
                  onPressed: _startScan,
                  child: const Text("Scan"),
                ),
                const SizedBox(width: 12),
                Text("Stav: $status"),
              ],
            ),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: _devices.length,
              itemBuilder: (_, i) {
                final d = _devices[i];
                return ListTile(
                  title: Text(d.name.isNotEmpty ? d.name : "(bez mena)"),
                  subtitle: Text("${d.id}  RSSI:${d.rssi}"),
                  onTap: () async {
                    await repo.stopScan();
                    await repo.connect(d.id);
                    if (mounted) setState(() {});
                  },
                );
              },
            ),
          ),
          if (repo.deviceId != null && status == BleStatus.connected)
            Padding(
              padding: const EdgeInsets.all(12),
              child: Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _msgCtrl,
                      decoration: const InputDecoration(labelText: "Správa → hodinky"),
                    ),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton(
                    onPressed: () async {
                      final txt = _msgCtrl.text.trim();
                      if (txt.isNotEmpty) {
                        await repo.sendString(txt);
                        _msgCtrl.clear();
                      }
                    },
                    child: const Text("Send"),
                  ),
                ],
              ),
            ),
          const SizedBox(height: 12),
        ],
      ),
    );
  }
}
