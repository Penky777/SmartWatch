// lib/services/ble/ble_repository.dart
import 'dart:async';
import 'dart:convert';

import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'package:shared_preferences/shared_preferences.dart';

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

  // ===== Console (log) =====
  final _consoleCtrl = StreamController<String>.broadcast();
  Stream<String> get console => _consoleCtrl.stream;

  final List<String> _consoleHistory = [];
  List<String> get consoleHistory => List.unmodifiable(_consoleHistory);

  void _log(String msg) {
    final line = '${DateTime.now().toIso8601String().substring(11, 19)}  $msg';
    _consoleHistory.insert(0, line);
    if (_consoleHistory.length > 500) _consoleHistory.removeLast();
    _consoleCtrl.add(line);
  }

  Stream<BleStatus> get status => _statusCtrl.stream;
  BleStatus get currentStatus => _status;
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

  // ===== Auto reconnect =====
  bool _autoReconnectEnabled = true;
  bool _manualDisconnect = false;
  Timer? _reconnectTimer;
  int _reconnectAttempts = 0;

  static const Duration _reconnectBaseDelay = Duration(seconds: 2);
  static const Duration _reconnectMaxDelay = Duration(seconds: 30);

  void _set(BleStatus s) {
    _status = s;
    _statusCtrl.add(s);
  }

  Duration _nextReconnectDelay() {
    // 2s, 4s, 8s, 16s, 30s...
    final secs = (_reconnectBaseDelay.inSeconds * (1 << _reconnectAttempts))
        .clamp(_reconnectBaseDelay.inSeconds, _reconnectMaxDelay.inSeconds);
    return Duration(seconds: secs);
  }

  void enableAutoReconnect(bool enabled) {
    _autoReconnectEnabled = enabled;
    _log('AUTO_RECONNECT = $enabled');
  }

  void dispose() {
    _scanTimeout?.cancel();
    _scanSub?.cancel();
    _connSub?.cancel();
    _notifySub?.cancel();
    _pairingTimeout?.cancel();
    _reconnectTimer?.cancel();

    _devicesCtrl.close();
    _pairingPinCtrl.close();
    _txTextCtrl.close();
    _consoleCtrl.close();
    _statusCtrl.close();
  }


  Future<void> startScan({
    Duration? timeout,
    bool filterService = false,
  }) async {
    await stopScan();

    _set(BleStatus.scanning);
    _log('SCAN start (filterService=$filterService)');

    final src =
    _client.scanForDevices(service: filterService ? BleUUIDs.service : null);

    _scanSub = src.listen(
          (d) => _devicesCtrl.add(d),
      onError: (e) {
        _log('SCAN ERROR: $e');
        _set(BleStatus.error);
      },
      onDone: () {
        _log('SCAN done');
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
    _log('SCAN stop');
  }

  Future<void> connect(String id) async {
    await stopScan();

    // ideme connectovať -> nie je to manuálne ukončenie
    _manualDisconnect = false;

    // uložiť posledné zariadenie pre background service / next app launch
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('ble_last_device', id);

    // zruš plánovaný reconnect, lebo ideme connectnúť hneď
    _reconnectTimer?.cancel();
    _reconnectTimer = null;

    _set(BleStatus.connecting);
    _deviceId = id;
    _txBuffer.clear();
    _pairingTimeout?.cancel();

    _log('CONNECTING to $id');

    await _connSub?.cancel();
    _connSub = _client.connectionStream(id).listen(
          (event) async {
        switch (event.connectionState) {
          case DeviceConnectionState.connecting:
            _set(BleStatus.connecting);
            break;

          case DeviceConnectionState.connected:
            _set(BleStatus.connected);
            _log('CONNECTED to $id');

            // reset reconnect attempts, lebo sme úspešne connected
            _reconnectAttempts = 0;

            // 1) MTU request pred notifikáciami
            try {
              final negotiated = await _client.requestMtu(id, 247);
              _log('MTU negotiated: $negotiated');
            } catch (e) {
              _log('MTU request failed (fallback ok): $e');
            }

            // 2) Subscribe na TX
            await _notifySub?.cancel();
            _notifySub = _client
                .subscribe(
              deviceId: id,
              service: BleUUIDs.service,
              characteristic: BleUUIDs.txChar,
            )
                .listen(
                  (data) {
                _log('TX chunk len=${data.length}');
                _handleTxNotify(data);
              },
              onError: (e) {
                _log('NOTIFY ERROR: $e');
                _set(BleStatus.error);
              },
            );

            _log('Subscribed to TX notifications');
            break;

          case DeviceConnectionState.disconnecting:
          // no-op
            break;

          case DeviceConnectionState.disconnected:
            _pairingTimeout?.cancel();
            _log('DISCONNECTED');
            _set(BleStatus.disconnected);

            // ✅ auto reconnect (len ak to nebolo manuálne)
            if (_autoReconnectEnabled && !_manualDisconnect) {
              final id2 = _deviceId;
              if (id2 != null) {
                _reconnectTimer?.cancel();
                final delay = _nextReconnectDelay();
                _log(
                    'Reconnect in ${delay.inSeconds}s (attempt ${_reconnectAttempts + 1})');
                _reconnectTimer = Timer(delay, () {
                  _reconnectAttempts = (_reconnectAttempts + 1).clamp(0, 10);
                  connect(id2);
                });
              }
            }
            break;
        }
      },
      onError: (e) {
        _log('CONNECT ERROR: $e');
        _set(BleStatus.error);

        if (_autoReconnectEnabled && !_manualDisconnect) {
          final id2 = _deviceId;
          if (id2 != null) {
            _reconnectTimer?.cancel();
            final delay = _nextReconnectDelay();
            _log(
                'Reconnect after error in ${delay.inSeconds}s (attempt ${_reconnectAttempts + 1})');
            _reconnectTimer = Timer(delay, () {
              _reconnectAttempts = (_reconnectAttempts + 1).clamp(0, 10);
              connect(id2);
            });
          }
        }
      },
    );
  }

  Future<void> disconnect() async {
    // manuálne odpojenie -> nerob auto reconnect
    _manualDisconnect = true;
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
    _reconnectAttempts = 0;

    _pairingTimeout?.cancel();
    await _notifySub?.cancel();
    await _connSub?.cancel();
    _notifySub = null;
    _connSub = null;
    _log('DISCONNECT (manual)');
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

    _log('PHONE -> WATCH: $text');
  }

  Future<void> confirmPairing() async {
    // po potvrdení už nechceme pairing timeout, aby to zostalo pripojené
    _pairingTimeout?.cancel();
    await sendString('confirm');
    _log('PAIRING confirmed');
  }

  Future<void> rejectPairing() async {
    _pairingTimeout?.cancel();
    _log('PAIRING rejected');
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
          _log('WATCH -> PHONE: {"pin":$pin}');
          _pairingPinCtrl.add(pin);

          // Pairing timeout (len kým user nepotvrdí)
          _pairingTimeout?.cancel();
          _pairingTimeout = Timer(pairingWindow, () async {
            _log('PAIRING timeout -> disconnect');
            await disconnect();
          });
        } else {
          // iné JSON správy
          _log('WATCH -> PHONE (json): $text');
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
