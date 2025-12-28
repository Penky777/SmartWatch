// lib/services/ble/ble_repository.dart
import 'dart:async';
import 'dart:convert';

import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'ble_client.dart';
import 'ble_uuids.dart';

enum BleStatus { idle, scanning, connecting, connected, disconnected, error }

class BleRepository {
  final BleClient _client;
  BleRepository(this._client);

  final _statusCtrl = StreamController<BleStatus>.broadcast();
  BleStatus _status = BleStatus.idle;

  final _devicesCtrl = StreamController<DiscoveredDevice>.broadcast();
  Stream<DiscoveredDevice> get scannedDevices => _devicesCtrl.stream;

  final _pairingPinCtrl = StreamController<String>.broadcast();
  Stream<String> get pairingPins => _pairingPinCtrl.stream;

  final _txTextCtrl = StreamController<String>.broadcast();
  Stream<String> get incomingText => _txTextCtrl.stream;

  Stream<BleStatus> get status => _statusCtrl.stream;
  String? get deviceId => _deviceId;

  String? _deviceId;

  StreamSubscription<DiscoveredDevice>? _scanSub;
  StreamSubscription<ConnectionStateUpdate>? _connSub;
  StreamSubscription<List<int>>? _notifySub;
  Timer? _scanTimeout;

  // Pairing
  Timer? _pairingTimeout;
  static const Duration pairingWindow = Duration(seconds: 30);

  // TX buffer (pre JSON čo príde po kusoch)
  final List<int> _txBuffer = [];
  static const int _maxBuffer = 4096;

  void _set(BleStatus s) {
    _status = s;
    _statusCtrl.add(s);
  }

  void dispose() {
    _scanTimeout?.cancel();
    _scanSub?.cancel();
    _connSub?.cancel();
    _notifySub?.cancel();
    _pairingTimeout?.cancel();

    _devicesCtrl.close();
    _pairingPinCtrl.close();
    _txTextCtrl.close();
    _statusCtrl.close();
  }

  /// Start scan; pushuje zariadenia do `scannedDevices`.
  /// - Ak `filterService=true`, použije sa service UUID filter (rýchlejšie/čistejšie).
  /// - Ak false, hľadá všetko (lepšie na debug, keď filter nesedí).
  Future<void> startScan({
    Duration? timeout,
    bool filterService = false,
  }) async {
    await stopScan();

    _set(BleStatus.scanning);

    final src = _client.scanForDevices(
      service: filterService ? BleUUIDs.service : null,
    );

    _scanSub = src.listen(
          (d) => _devicesCtrl.add(d),
      onError: (_) => _set(BleStatus.error),
      onDone: () {
        if (_status == BleStatus.scanning) _set(BleStatus.idle);
      },
    );

    if (timeout != null) {
      _scanTimeout?.cancel();
      _scanTimeout = Timer(timeout, stopScan);
    }
  }

  Future<void> stopScan() async {
    await _scanSub?.cancel();
    _scanSub = null;
    _scanTimeout?.cancel();
    _scanTimeout = null;
    if (_status == BleStatus.scanning) _set(BleStatus.idle);
  }

  Future<void> connect(String id) async {
    await stopScan();

    _set(BleStatus.connecting);
    _deviceId = id;

    _txBuffer.clear();
    _pairingTimeout?.cancel();

    await _connSub?.cancel();
    _connSub = _client.connectionStream(id).listen(
          (event) async {
        switch (event.connectionState) {
          case DeviceConnectionState.connecting:
            _set(BleStatus.connecting);
            break;

          case DeviceConnectionState.connected:
            _set(BleStatus.connected);

            // ✅ MTU request pred NOTIFY
            try {
              final negotiated = await _client.requestMtu(id, 247);
              // voliteľne push do log streamu
              if (negotiated != null) {
                _txTextCtrl.add('MTU negotiated: $negotiated');
              }
            } catch (e) {
              _txTextCtrl.add('MTU request failed (fallback ok): $e');
            }

            // ✅ SUBSCRIBE na TX
            await _notifySub?.cancel();
            _notifySub = _client
                .subscribe(
              deviceId: id,
              service: BleUUIDs.service,
              characteristic: BleUUIDs.txChar,
            )
                .listen(
                  (data) {
                _handleTxNotify(data);
              },
              onError: (_) => _set(BleStatus.error),
            );
            break;

          case DeviceConnectionState.disconnected:
            _pairingTimeout?.cancel();
            _set(BleStatus.disconnected);
            break;

          case DeviceConnectionState.disconnecting:
          // no-op
            break;
        }
      },
      onError: (_) => _set(BleStatus.error),
    );
  }

  Future<void> disconnect() async {
    _pairingTimeout?.cancel();
    await _notifySub?.cancel();
    await _connSub?.cancel();
    _notifySub = null;
    _connSub = null;
    _set(BleStatus.disconnected);
  }

  Future<void> sendString(String text) async {
    final id = _deviceId;
    if (id == null) throw StateError("Device not connected");
    final bytes = utf8.encode(text);
    await _client.write(
      deviceId: id,
      service: BleUUIDs.service,
      characteristic: BleUUIDs.rxChar,
      value: bytes,
      withResponse: true,
    );
  }

  Future<void> confirmPairing() async {
    _pairingTimeout?.cancel();
    await sendString('confirm');
  }

  Future<void> rejectPairing() async {
    _pairingTimeout?.cancel();
    // voliteľne: await sendString('reject');
    await disconnect();
  }

  // ====== TX notify handling ======

  void _handleTxNotify(List<int> chunk) {
    if (chunk.isEmpty) return;

    // Poistka na buffer
    if (_txBuffer.length + chunk.length > _maxBuffer) {
      _txBuffer.clear();
    }
    _txBuffer.addAll(chunk);

    // Nájdeme JSON objekt { ... } (robustné aj pri chunkovaní)
    while (true) {
      final start = _txBuffer.indexOf(123); // '{'
      if (start == -1) {
        _txBuffer.clear();
        return;
      }
      if (start > 0) _txBuffer.removeRange(0, start);

      final end = _txBuffer.lastIndexOf(125); // '}'
      if (end == -1 || end <= 0) {
        // ešte nemáme celý JSON
        return;
      }

      final candidate = _txBuffer.sublist(0, end + 1);
      final text = _safeUtf8(candidate).trim();

      try {
        final obj = jsonDecode(text);
        _txBuffer.removeRange(0, end + 1);

        if (obj is Map && obj['pin'] != null) {
          final pin = obj['pin'].toString();
          _pairingPinCtrl.add(pin);

          // Start/refresh pairing timeout
          _pairingTimeout?.cancel();
          _pairingTimeout = Timer(pairingWindow, () async {
            await disconnect();
          });
        } else {
          // iné JSON správy si môžeš posielať ďalej
          _txTextCtrl.add(text);
        }

        continue;
      } catch (_) {
        // Nevalidné -> posuň a skús znovu, aby sme sa nezasekli
        _txBuffer.removeAt(0);
        continue;
      }
    }
  }

  String _safeUtf8(List<int> data) {
    try {
      return utf8.decode(data, allowMalformed: true);
    } catch (_) {
      return data.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ');
    }
  }
}
