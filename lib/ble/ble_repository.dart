// lib/ble/ble_repository.dart - S HIVE + RESET BONDING
import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'package:rxdart/rxdart.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'ble_client.dart';
import 'ble_uuids.dart';
import '../services/bluetooth_bond_service.dart';  // ✅ PRIDANÉ

enum BleStatus { idle, scanning, connecting, connected, disconnected, error }

/// Trieda pre health dáta z hodiniek
class HealthData {
  int heartRate;
  int spo2;
  int steps;
  int minHr;
  int maxHr;
  final List<int> heartRateHistory;
  final List<int> spo2History;

  static const int maxHistory = 50;

  HealthData()
      : heartRate = 0,
        spo2 = 0,
        steps = 0,
        minHr = 0,
        maxHr = 0,
        heartRateHistory = [],
        spo2History = [];

  void update(Map<String, dynamic> data) {
    final hr = data['heartRate'] as int?;
    final sp = data['spo2'] as int?;
    final st = data['steps'] as int?;

    if (hr != null && hr > 0) {
      heartRate = hr;
      heartRateHistory.add(hr);
      if (heartRateHistory.length > maxHistory) {
        heartRateHistory.removeAt(0);
      }
      if (minHr == 0 || hr < minHr) minHr = hr;
      if (hr > maxHr) maxHr = hr;
    }

    if (sp != null && sp > 0) {
      spo2 = sp;
      spo2History.add(sp);
      if (spo2History.length > maxHistory) {
        spo2History.removeAt(0);
      }
    }

    if (st != null) {
      steps = st;
    }
  }

  void clear() {
    heartRate = 0;
    spo2 = 0;
    steps = 0;
    minHr = 0;
    maxHr = 0;
    heartRateHistory.clear();
    spo2History.clear();
  }
}

class BleRepository {
  final BleClient _client;
  BleRepository(this._client);

  static const String kWatchId = 'ble_last_device';

  // HIVE BOX pre históriu
  static const String _hiveBoxName = 'health_data';
  Box? _hiveBox;

  final _statusCtrl = BehaviorSubject<BleStatus>.seeded(BleStatus.idle);
  BleStatus _status = BleStatus.idle;

  final _pairedStatusCtrl = BehaviorSubject<bool>.seeded(false);
  Stream<bool> get pairedStatus => _pairedStatusCtrl.stream;

  final HealthData healthData = HealthData();
  final _healthDataCtrl = BehaviorSubject<HealthData?>.seeded(null);
  Stream<HealthData?> get healthDataStream => _healthDataCtrl.stream;

  final _devicesCtrl = StreamController<DiscoveredDevice>.broadcast();
  Stream<DiscoveredDevice> get scannedDevices => _devicesCtrl.stream;

  final _pairingPinCtrl = StreamController<String>.broadcast();
  Stream<String> get pairingPins => _pairingPinCtrl.stream;

  final _txTextCtrl = StreamController<String>.broadcast();
  Stream<String> get incomingText => _txTextCtrl.stream;

  final _consoleCtrl = StreamController<String>.broadcast();
  Stream<String> get console => _consoleCtrl.stream;

  final List<String> _consoleHistory = [];
  List<String> get consoleHistory => List.unmodifiable(_consoleHistory);

  void _log(String msg) {
    final line = '${DateTime.now().toIso8601String().substring(11, 19)}  $msg';
    _consoleHistory.insert(0, line);
    if (_consoleHistory.length > 500) _consoleHistory.removeLast();
    _consoleCtrl.add(line);
    debugPrint('BLE: $msg');
  }

  Stream<BleStatus> get status => _statusCtrl.stream;
  BleStatus get currentStatus => _status;
  String? get deviceId => _deviceId;
  bool get isPaired => _isPaired;

  String? _deviceId;

  StreamSubscription<DiscoveredDevice>? _scanSub;
  StreamSubscription<ConnectionStateUpdate>? _connSub;
  StreamSubscription<List<int>>? _notifySub;
  Timer? _scanTimeout;

  Timer? _timeSyncTimer;
  static const Duration _timeSyncPeriod = Duration(hours: 12);

  bool _isPaired = false;
  String _pairedKey(String id) => 'ble_paired_$id';

  Timer? _pairingTimeout;
  static const Duration pairingWindow = Duration(seconds: 30);

  final List<int> _txBuffer = [];
  static const int _maxBuffer = 4096;

  bool _autoReconnectEnabled = true;
  bool _manualDisconnect = false;
  Timer? _reconnectTimer;
  int _reconnectAttempts = 0;

  static const Duration _reconnectBaseDelay = Duration(seconds: 2);
  static const Duration _reconnectMaxDelay = Duration(seconds: 30);

  // ========== HIVE INICIALIZÁCIA A NAČÍTANIE ==========

  Future<void> initHive() async {
    try {
      _hiveBox = await Hive.openBox(_hiveBoxName);
      _loadHistoryFromHive();
      _log('HIVE: Initialized, loaded ${healthData.heartRateHistory.length} HR values');
    } catch (e) {
      _log('HIVE ERROR: $e');
    }
  }

  void _loadHistoryFromHive() {
    if (_hiveBox == null) return;

    final hrList = _hiveBox!.get('heartRateHistory');
    if (hrList != null && hrList is List) {
      healthData.heartRateHistory.clear();
      healthData.heartRateHistory.addAll(List<int>.from(hrList));
    }

    final spo2List = _hiveBox!.get('spo2History');
    if (spo2List != null && spo2List is List) {
      healthData.spo2History.clear();
      healthData.spo2History.addAll(List<int>.from(spo2List));
    }

    healthData.heartRate = _hiveBox!.get('lastHeartRate') ?? 0;
    healthData.spo2 = _hiveBox!.get('lastSpo2') ?? 0;
    healthData.steps = _hiveBox!.get('lastSteps') ?? 0;
    healthData.minHr = _hiveBox!.get('minHr') ?? 0;
    healthData.maxHr = _hiveBox!.get('maxHr') ?? 0;

    _healthDataCtrl.add(healthData);
  }

  Future<void> _saveHistoryToHive() async {
    if (_hiveBox == null) return;

    try {
      await _hiveBox!.put('heartRateHistory', healthData.heartRateHistory.toList());
      await _hiveBox!.put('spo2History', healthData.spo2History.toList());
      await _hiveBox!.put('lastHeartRate', healthData.heartRate);
      await _hiveBox!.put('lastSpo2', healthData.spo2);
      await _hiveBox!.put('lastSteps', healthData.steps);
      await _hiveBox!.put('minHr', healthData.minHr);
      await _hiveBox!.put('maxHr', healthData.maxHr);
      await _hiveBox!.put('lastSaved', DateTime.now().toIso8601String());
    } catch (e) {
      _log('HIVE SAVE ERROR: $e');
    }
  }

  // ========== STATUS HELPERS ==========

  void _set(BleStatus s) {
    _status = s;
    _statusCtrl.add(s);
  }

  void _setPaired(bool value) {
    _isPaired = value;
    _pairedStatusCtrl.add(value);
  }

  Future<bool> loadPairedForWatch() async {
    final prefs = await SharedPreferences.getInstance();
    final savedId = prefs.getString(kWatchId);
    if (savedId == null) {
      _setPaired(false);
      return false;
    }
    final paired = prefs.getBool(_pairedKey(savedId)) ?? false;
    _setPaired(paired);
    _log('loadPairedForWatch: $savedId -> paired=$paired');
    return paired;
  }

  Duration _nextReconnectDelay() {
    final secs = (_reconnectBaseDelay.inSeconds * (1 << _reconnectAttempts))
        .clamp(_reconnectBaseDelay.inSeconds, _reconnectMaxDelay.inSeconds);
    return Duration(seconds: secs);
  }

  void enableAutoReconnect(bool enabled) {
    _autoReconnectEnabled = enabled;
    _log('AUTO_RECONNECT = $enabled');
  }

  // ========== SCAN ==========

  Future<void> startScan({
    Duration? timeout,
    bool filterService = false,
  }) async {
    await stopScan();

    _set(BleStatus.scanning);
    _log('SCAN start (filterService=$filterService)');

    final src = _client.scanForDevices(service: filterService ? BleUUIDs.service : null);

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

  // ========== CONNECT ==========

  Future<void> connect(String id) async {
    await stopScan();

    _manualDisconnect = false;

    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(kWatchId, id);

    final paired = prefs.getBool(_pairedKey(id)) ?? false;
    _setPaired(paired);
    _log('PAIRED flag for $id = $_isPaired');

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
            _reconnectAttempts = 0;

            try {
              final negotiated = await _client.requestMtu(id, 247);
              _log('MTU negotiated: $negotiated');
            } catch (e) {
              _log('MTU request failed: $e');
            }

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

            if (_isPaired) {
              _startTimeSync();
            }
            break;

          case DeviceConnectionState.disconnecting:
            break;

          case DeviceConnectionState.disconnected:
            _stopTimeSync();
            _pairingTimeout?.cancel();

            await _saveHistoryToHive();
            _log('HIVE: History saved on disconnect');

            _log('DISCONNECTED');
            _set(BleStatus.disconnected);

            if (_autoReconnectEnabled && !_manualDisconnect) {
              final id2 = _deviceId;
              if (id2 != null) {
                _reconnectTimer?.cancel();
                final delay = _nextReconnectDelay();
                _log('Reconnect in ${delay.inSeconds}s (attempt ${_reconnectAttempts + 1})');
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
            _log('Reconnect after error in ${delay.inSeconds}s');
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
    _manualDisconnect = true;
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
    _reconnectAttempts = 0;

    _stopTimeSync();

    await _saveHistoryToHive();
    _log('HIVE: History saved on manual disconnect');

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

  // ========== PAIRING ==========

  Future<void> confirmPairing() async {
    _pairingTimeout?.cancel();
    await sendString('confirm');
    _log('PAIRING confirmed');

    final id = _deviceId;
    if (id != null) {
      final prefs = await SharedPreferences.getInstance();
      _setPaired(true);
      await prefs.setBool(_pairedKey(id), true);
      _log('PAIRED saved for $id = true');

      _startTimeSync();
    }
  }

  Future<void> rejectPairing() async {
    _pairingTimeout?.cancel();
    _log('PAIRING rejected');

    final id = _deviceId;
    if (id != null) {
      final prefs = await SharedPreferences.getInstance();
      _setPaired(false);
      await prefs.setBool(_pairedKey(id), false);
      _log('PAIRED saved for $id = false');
    }

    await disconnect();
  }

  // ========== ✅ RESET BONDING (NOVÉ) ==========

  /// Odstráni bonding a odpojí sa od zariadenia
  /// Používateľ bude musieť znova spárovať (PIN)
  Future<bool> resetBonding() async {
    final id = _deviceId;
    if (id == null) {
      _log('RESET_BONDING: No device connected');
      return false;
    }

    _log('RESET_BONDING: Starting for $id');

    // 1. Odpoj sa
    await disconnect();

    // 2. Vymaž lokálne uložený pairing flag
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_pairedKey(id), false);
    _setPaired(false);
    _log('RESET_BONDING: Local pairing cleared');

    // 3. Odstráň Android bonding
    final success = await BluetoothBondService.removeBond(id);
    _log('RESET_BONDING: Android removeBond = $success');

    // 4. Vymaž uložené device ID
    await prefs.remove(kWatchId);
    _deviceId = null;

    return success;
  }

  // ========== TX HANDLING ==========

  void _handleTxNotify(List<int> chunk) {
    if (chunk.isEmpty) return;

    if (_txBuffer.length + chunk.length > _maxBuffer) {
      _txBuffer.clear();
    }
    _txBuffer.addAll(chunk);

    while (true) {
      final start = _txBuffer.indexOf(123);
      if (start == -1) {
        _txBuffer.clear();
        return;
      }
      if (start > 0) _txBuffer.removeRange(0, start);

      final end = _txBuffer.lastIndexOf(125);
      if (end == -1 || end <= 0) {
        return;
      }

      final candidate = _txBuffer.sublist(0, end + 1);
      final text = _safeUtf8(candidate).trim();

      try {
        final obj = jsonDecode(text);
        _txBuffer.removeRange(0, end + 1);

        if (obj is Map<String, dynamic>) {
          if (obj['pin'] != null) {
            final pin = obj['pin'].toString();
            _log('WATCH -> PHONE: {"pin":$pin}');
            _pairingPinCtrl.add(pin);

            _pairingTimeout?.cancel();
            _pairingTimeout = Timer(pairingWindow, () async {
              _log('PAIRING timeout -> disconnect');
              await disconnect();
            });
          } else if (obj.containsKey('heartRate') ||
              obj.containsKey('spo2') ||
              obj.containsKey('steps')) {
            _log('WATCH -> PHONE (health): $text');
            healthData.update(obj);
            _healthDataCtrl.add(healthData);
            _txTextCtrl.add(text);

            _saveHistoryToHive();
          } else {
            _log('WATCH -> PHONE (json): $text');
            _txTextCtrl.add(text);
          }
        }

        continue;
      } catch (_) {
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

  // ========== TIME SYNC ==========

  void _startTimeSync() {
    _timeSyncTimer?.cancel();
    _sendTimeSyncOnce();
    _timeSyncTimer = Timer.periodic(_timeSyncPeriod, (_) {
      _sendTimeSyncOnce();
    });
    _log('TIME_SYNC started (every ${_timeSyncPeriod.inHours}h)');
  }

  void _stopTimeSync() {
    _timeSyncTimer?.cancel();
    _timeSyncTimer = null;
    _log('TIME_SYNC stopped');
  }

  Future<void> _sendTimeSyncOnce() async {
    final id = _deviceId;
    if (id == null) return;
    if (_status != BleStatus.connected) return;
    if (!_isPaired) {
      _log('TIME_SYNC skipped (not paired yet)');
      return;
    }

    final now = DateTime.now();
    final ts = now.toUtc().millisecondsSinceEpoch ~/ 1000;
    final tzMin = now.timeZoneOffset.inMinutes;

    final msg = {
      "type": "sync",
      "ts": ts,
      "tzMin": tzMin,
    };

    try {
      await sendString(jsonEncode(msg));
      _log('TIME_SYNC sent');
    } catch (e) {
      _log('TIME_SYNC send failed: $e');
    }
  }

  // ========== DISPOSE ==========

  void dispose() {
    _saveHistoryToHive();
    _hiveBox?.close();

    _scanTimeout?.cancel();
    _scanSub?.cancel();
    _connSub?.cancel();
    _notifySub?.cancel();
    _pairingTimeout?.cancel();
    _reconnectTimer?.cancel();
    _timeSyncTimer?.cancel();

    _devicesCtrl.close();
    _pairingPinCtrl.close();
    _txTextCtrl.close();
    _consoleCtrl.close();
    _statusCtrl.close();
    _pairedStatusCtrl.close();
    _healthDataCtrl.close();
  }
}