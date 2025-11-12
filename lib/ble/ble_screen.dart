import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' hide BleStatus;

import '../../ble/ble_client.dart';
import '../../ble/ble_permissions.dart';
import '../../ble/ble_repository.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/ble/device_tile.dart';
import '../../widgets/status_badge.dart';

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
        if (mounted) setState(() => _devices.add(d));
      }
    });
  }

  @override
  void dispose() {
    repo.dispose();
    _msgCtrl.dispose();
    super.dispose();
  }

  Widget _statusBadge(BleStatus s) {
    switch (s) {
      case BleStatus.connected:
        return const StatusBadge.connected();
      case BleStatus.connecting:
        return const StatusBadge.connecting();
      case BleStatus.scanning:
        return const StatusBadge.custom(text: "Skenujem…", color: Colors.blue);
      case BleStatus.disconnected:
        return const StatusBadge.disconnected();
      case BleStatus.error:
        return const StatusBadge.custom(text: "Chyba", color: Colors.redAccent);
      case BleStatus.idle:
      default:
        return const StatusBadge.custom(text: "Pripravené", color: Colors.grey);
    }
  }

  String _statusLabel(BleStatus s) {
    switch (s) {
      case BleStatus.scanning:
        return "Skenujem…";
      case BleStatus.connecting:
        return "Pripájanie…";
      case BleStatus.connected:
        return "Pripojené";
      case BleStatus.disconnected:
        return "Odpojené";
      case BleStatus.error:
        return "Chyba";
      case BleStatus.idle:
      default:
        return "Pripravené";
    }
  }

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;

    return AppScaffold(
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
          // HLAVIČKA – ListTile bez overflowu
          Padding(
            padding: const EdgeInsets.fromLTRB(8, 8, 8, 4),
            child: Material(
              color: Colors.transparent,
              child: ListTile(
                dense: true,
                contentPadding: const EdgeInsets.symmetric(horizontal: 8),
                leading: StreamBuilder<BleStatus>(
                  stream: repo.status,
                  initialData: BleStatus.idle,
                  builder: (context, snap) {
                    final s = snap.data ?? BleStatus.idle;
                    return _statusBadge(s);
                  },
                ),
                title: StreamBuilder<BleStatus>(
                  stream: repo.status,
                  initialData: BleStatus.idle,
                  builder: (context, snap) {
                    final s = snap.data ?? BleStatus.idle;
                    return Text(
                      "Stav: ${_statusLabel(s)}",
                      maxLines: 1,
                      softWrap: false,
                      overflow: TextOverflow.ellipsis,
                    );
                  },
                ),
                trailing: StreamBuilder<BleStatus>(
                  stream: repo.status,
                  initialData: BleStatus.idle,
                  builder: (context, snap) {
                    final connected =
                        snap.data == BleStatus.connected && repo.deviceId != null;
                    if (!connected) return const SizedBox.shrink();

                    final id = repo.deviceId!;
                    final short =
                    id.length > 8 ? "…${id.substring(id.length - 8)}" : id;

                    return ConstrainedBox(
                      constraints: const BoxConstraints(maxWidth: 140),
                      child: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Icon(Icons.watch, size: 18, color: cs.primary),
                          const SizedBox(width: 6),
                          Flexible(
                            child: Text(
                              short,
                              maxLines: 1,
                              softWrap: false,
                              overflow: TextOverflow.ellipsis,
                              style: const TextStyle(fontWeight: FontWeight.w600),
                            ),
                          ),
                        ],
                      ),
                    );
                  },
                ),
              ),
            ),
          ),

          // ZOZNAM ZARIADENÍ
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
                  },
                );
              },
            ),
          ),

          // ODOSEL SPRÁVY
          StreamBuilder<BleStatus>(
            stream: repo.status,
            initialData: BleStatus.idle,
            builder: (context, snap) {
              final isConnected =
                  snap.data == BleStatus.connected && repo.deviceId != null;
              if (!isConnected) return const SizedBox(height: 8);
              return Padding(
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
              );
            },
          ),
        ],
      ),
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
