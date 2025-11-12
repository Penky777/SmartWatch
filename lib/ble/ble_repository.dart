// lib/services/ble/ble_repository.dart
import 'dart:async';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'ble_client.dart';
import 'ble_uuids.dart';

enum BleStatus { idle, scanning, connecting, connected, disconnected, error }

class BleRepository {
  final BleClient _client;
  BleRepository(this._client);

  final _statusCtrl = StreamController<BleStatus>.broadcast();
  BleStatus _status = BleStatus.idle;
  String? _deviceId;

  StreamSubscription<DiscoveredDevice>? _scanSub;
  StreamSubscription<ConnectionStateUpdate>? _connSub;
  StreamSubscription<List<int>>? _notifySub;
  Timer? _scanTimeout;

  Stream<BleStatus> get status => _statusCtrl.stream;
  String? get deviceId => _deviceId;

  void _set(BleStatus s) {
    _status = s;
    _statusCtrl.add(s);
  }

  void dispose() {
    _scanTimeout?.cancel();
    _scanSub?.cancel();
    _connSub?.cancel();
    _notifySub?.cancel();
    _statusCtrl.close();
  }

  /// Spusti scan a vráť broadcast stream zariadení.
  /// Repo si drží subscription (cez onListen), aby šlo scan neskôr stopnúť.
  Stream<DiscoveredDevice> startScan({Duration? timeout}) {
    _set(BleStatus.scanning);

    final src = _client.scanForDevices(service: BleUUIDs.service);

    // Z broadcastu vieme zachytiť subscription aj zrušenie.
    final bcast = src.asBroadcastStream(
      onListen: (sub) {
        _scanSub = sub;
        if (timeout != null) {
          _scanTimeout?.cancel();
          _scanTimeout = Timer(timeout, stopScan);
        }
      },
      onCancel: (sub) {
        _scanSub = null;
        _scanTimeout?.cancel();
        _scanTimeout = null;
        if (_status == BleStatus.scanning) _set(BleStatus.idle);
      },
    );

    return bcast;
  }

  Future<void> stopScan() async {
    await _scanSub?.cancel();
    _scanSub = null;
    _scanTimeout?.cancel();
    _scanTimeout = null;
    if (_status == BleStatus.scanning) _set(BleStatus.idle);
  }

  Future<void> connect(String id) async {
    // pre istotu ukonči scan
    await stopScan();

    _set(BleStatus.connecting);
    _deviceId = id;

    await _connSub?.cancel();
    _connSub = _client.connectionStream(id).listen((event) async {
      switch (event.connectionState) {
        case DeviceConnectionState.connected:
          _set(BleStatus.connected);
          // MTU pre väčšie payloady (nie je garantované)
          try { await _client.requestMtu(id, 247); } catch (_) {}
          // SUBSCRIBE na notifikácie z TX
          await _notifySub?.cancel();
          _notifySub = _client.subscribe(
            deviceId: id,
            service: BleUUIDs.service,
            characteristic: BleUUIDs.txChar,
          ).listen(
                (data) {
              // TODO: propaguj ďalej (napr. cez vlastný StreamController)
            },
            onError: (_) => _set(BleStatus.error),
          );
          break;

        case DeviceConnectionState.disconnected:
          _set(BleStatus.disconnected);
          break;

        case DeviceConnectionState.connecting:
          _set(BleStatus.connecting);
          break;

        case DeviceConnectionState.disconnecting:
        // no-op
          break;
      }
    }, onError: (_) => _set(BleStatus.error));
  }

  Future<void> disconnect() async {
    await _connSub?.cancel();
    await _notifySub?.cancel();
    _connSub = null;
    _notifySub = null;
    _set(BleStatus.disconnected);
  }

  /// Spustí bonding automaticky pri prvom zápise, ak je characteristic "encrypted".
  Future<void> sendString(String text) async {
    final id = _deviceId;
    if (id == null) throw StateError("Device not connected");
    final bytes = List<int>.from(text.codeUnits);
    await _client.write(
      deviceId: id,
      service: BleUUIDs.service,
      characteristic: BleUUIDs.rxChar,
      value: bytes,
    );
  }
}
