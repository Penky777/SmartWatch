// lib/ble/lib/ble_screen.dart

import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import 'ble_permissions.dart';
import '../../widgets/app_scaffold.dart';
import '../../widgets/status_badge.dart';

class BleScreen extends StatefulWidget {
  const BleScreen({super.key});

  @override
  State<BleScreen> createState() => _BleScreenState();
}

class _BleScreenState extends State<BleScreen> {
  // ==== UUIDs z tvojho zadania ====
  static final Uuid _svcUuid =
  Uuid.parse('12345678-9abc-def0-1234-567890abcdef');

  // RX = phone -> watch (WRITE)
  static final Uuid _rxUuid =
  Uuid.parse('abcdef12-3456-789a-bcde-f01234567890');

  // TX = watch -> phone (NOTIFY)
  static final Uuid _txUuid =
  Uuid.parse('abcdef12-3456-789a-bcde-f01234567891');

  final _ble = FlutterReactiveBle();

  // Scanning
  StreamSubscription<DiscoveredDevice>? _scanSub;
  final List<DiscoveredDevice> _devices = [];
  bool _isScanning = false;

  // Connection
  StreamSubscription<ConnectionStateUpdate>? _connSub;
  StreamSubscription<List<int>>? _notifySub;
  String? _deviceId;
  bool _connecting = false;
  bool _connected = false;

  // UI log + input
  final List<String> _log = [];
  final _inputCtrl = TextEditingController();

  // Qualified characteristics (setnú sa po connecte)
  QualifiedCharacteristic? _rxChar;
  QualifiedCharacteristic? _txChar;

  @override
  void initState() {
    super.initState();
    _startScan();
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    _notifySub?.cancel();
    _connSub?.cancel();
    _inputCtrl.dispose();
    super.dispose();
  }

  // ============ Scan ============
  Future<void> _startScan() async {
    final ok = await ensureBlePermissions();
    if (!ok) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Bez povolení neviem hľadať BLE zariadenia.')),
      );
      return;
    }

    final status = await _ble.statusStream.first;
    if (status != BleStatus.ready) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Bluetooth nie je pripravený: $status')),
      );
      return;
    }

    setState(() {
      _isScanning = true;
      _devices.clear();
    });

    _scanSub?.cancel();

    _scanSub = _ble
        .scanForDevices(
      // DÔLEŽITÉ: ak chceš filtrovať LEN tvoju službu, odkomentuj:
      // withServices: [_svcUuid],
      withServices: const [],
      scanMode: ScanMode.lowLatency,
    )
        .listen(
          (d) {
        if (d.name.isEmpty) return; // nechávam len pomenované zariadenia
        final i = _devices.indexWhere((x) => x.id == d.id);
        setState(() {
          if (i == -1) {
            _devices.add(d);
          } else {
            _devices[i] = d;
          }
        });
      },
      onError: (e) {
        if (!mounted) return;
        setState(() => _isScanning = false);
        _pushLog('SCAN ERROR: $e');
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

  // ============ Connect ============
  Future<void> _connect(String id) async {
    // Odpoj staré spojenie
    await _disconnect();

    setState(() {
      _deviceId = id;
      _connecting = true;
      _connected = false;
    });

    _pushLog('Connecting to $id ...');

    _connSub = _ble
        .connectToDevice(
      id: id,
      connectionTimeout: const Duration(seconds: 10),
    )
        .listen(
          (update) async {
        switch (update.connectionState) {
          case DeviceConnectionState.connecting:
            setState(() {
              _connecting = true;
              _connected = false;
            });
            break;

          case DeviceConnectionState.connected:
            _pushLog('Connected.');
            setState(() {
              _connecting = false;
              _connected = true;
            });

            // Nastav charakteristiky
            _rxChar = QualifiedCharacteristic(
              deviceId: id,
              serviceId: _svcUuid,
              characteristicId: _rxUuid,
            );
            _txChar = QualifiedCharacteristic(
              deviceId: id,
              serviceId: _svcUuid,
              characteristicId: _txUuid,
            );

            // (voliteľne) discovery – nie je nutné pre reactive_ble, ale pri debugu sa zíde
            // final services = await _ble.discoverServices(id);
            // _pushLog('Discovered ${services.length} services');

            // Subscribe na TX (NOTIFY z hodiniek)
            _subscribeToTx();
            break;

          case DeviceConnectionState.disconnecting:
            setState(() {
              _connecting = true;
              _connected = false;
            });
            break;

          case DeviceConnectionState.disconnected:
            _pushLog('Disconnected.');
            setState(() {
              _connecting = false;
              _connected = false;
            });
            _notifySub?.cancel();
            break;
        }
      },
      onError: (e) {
        _pushLog('CONNECT ERROR: $e');
        setState(() {
          _connecting = false;
          _connected = false;
        });
      },
    );
  }

  Future<void> _disconnect() async {
    await _notifySub?.cancel();
    await _connSub?.cancel();
    setState(() {
      _connecting = false;
      _connected = false;
      _deviceId = null;
      _rxChar = null;
      _txChar = null;
    });
  }

  void _subscribeToTx() {
    final tx = _txChar;
    if (tx == null) return;

    _notifySub?.cancel();
    _notifySub = _ble.subscribeToCharacteristic(tx).listen(
          (data) {
        // predpokladáme text/UTF-8; ak posielate binár, uprav podľa potreby
        final msg = _safeUtf8(data);
        _pushLog('WATCH -> PHONE: $msg');
      },
      onError: (e) {
        _pushLog('NOTIFY ERROR: $e');
      },
    );
    _pushLog('Subscribed to TX notifications.');
  }

  Future<void> _sendToWatch(String text) async {
    final rx = _rxChar;
    if (!_connected || rx == null) {
      _pushLog('Nie som pripojený.');
      return;
    }
    final payload = utf8.encode(text);
    try {
      // ak máš na RX povolené Write With Response, je lepšie použiť túto metódu:
      await _ble.writeCharacteristicWithResponse(rx, value: payload);
      // alternatíva bez response: await _ble.writeCharacteristicWithoutResponse(rx, value: payload);
      _pushLog('PHONE -> WATCH: $text');
    } catch (e) {
      _pushLog('WRITE ERROR: $e');
    }
  }

  // ============ UI pomocníci ============
  void _pushLog(String line) {
    setState(() {
      _log.insert(0, '${DateTime.now().toIso8601String().substring(11, 19)}  $line');
    });
  }

  String _safeUtf8(List<int> data) {
    try {
      return utf8.decode(data);
    } catch (_) {
      return data.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ');
    }
  }

  // ============ UI ============
  @override
  Widget build(BuildContext context) {
    return AppScaffold(
      title: 'BLE hodinky',
      actions: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8),
          child: _connecting
              ? const StatusBadge.connecting()
              : _connected
              ? const StatusBadge.connected()
              : const StatusBadge.disconnected(),
        ),
      ],
      body: Column(
        children: [
          const SizedBox(height: 8),
          // Scan / Stop
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

          // Zoznam zariadení (ak nie som pripojený)
          if (!_connected) Expanded(child: _buildDeviceList()),

          // Sekcia pripojenia a konzoly (ak som pripojený)
          if (_connected)
            Expanded(
              child: Column(
                children: [
                  _buildConnectedHeader(),
                  const SizedBox(height: 8),
                  _buildConsole(),
                  _buildInputBar(),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildDeviceList() {
    if (_devices.isEmpty) {
      return const Center(
        child: Text(
          'Zatiaľ nič.\nSkontroluj, že hodinky vysielajú advertising\nalebo zapni filter na service UUID.',
          textAlign: TextAlign.center,
        ),
      );
    }

    return ListView.builder(
      itemCount: _devices.length,
      itemBuilder: (context, index) {
        final d = _devices[index];
        return ListTile(
          leading: const Icon(Icons.watch),
          title: Text(d.name),
          subtitle: Text('ID: ${d.id}\nRSSI: ${d.rssi} dBm'),
          isThreeLine: true,
          onTap: () => _connect(d.id),
        );
      },
    );
  }

  Widget _buildConnectedHeader() {
    return ListTile(
      leading: const Icon(Icons.link),
      title: Text('Pripojené k: ${_deviceId ?? "-"}'),
      trailing: TextButton.icon(
        onPressed: _disconnect,
        icon: const Icon(Icons.link_off),
        label: const Text('Odpojiť'),
      ),
    );
  }

  Widget _buildConsole() {
    return Expanded(
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 12),
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(12),
          color: Theme.of(context).colorScheme.surfaceVariant.withOpacity(0.25),
          border: Border.all(
            color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
          ),
        ),
        child: _log.isEmpty
            ? const Align(
          alignment: Alignment.topLeft,
          child: Text('Konzola je prázdna.'),
        )
            : ListView.separated(
          reverse: true,
          itemCount: _log.length,
          separatorBuilder: (_, __) => const Divider(height: 12),
          itemBuilder: (_, i) => Text(_log[i]),
        ),
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
              controller: _inputCtrl,
              decoration: const InputDecoration(
                hintText: 'Správa pre hodinky…',
                border: OutlineInputBorder(),
                isDense: true,
              ),
              onSubmitted: _handleSend,
            ),
          ),
          const SizedBox(width: 8),
          ElevatedButton.icon(
            onPressed: () => _handleSend(_inputCtrl.text),
            icon: const Icon(Icons.send),
            label: const Text('Poslať'),
          ),
        ],
      ),
    );
  }

  void _handleSend(String text) {
    final trimmed = text.trim();
    if (trimmed.isEmpty) return;
    _inputCtrl.clear();
    _sendToWatch(trimmed);
  }
}
