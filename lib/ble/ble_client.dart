// lib/services/ble/ble_client.dart
import 'dart:async';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

class BleClient {
  final _ble = FlutterReactiveBle();

  Stream<DiscoveredDevice> scanForDevices({required Uuid service}) {
    return _ble.scanForDevices(withServices: [service], scanMode: ScanMode.balanced);
  }

  Future<ConnectionStateUpdate> connect(String deviceId, {Duration timeout = const Duration(seconds: 12)}) async {
    late final StreamSubscription<ConnectionStateUpdate> sub;
    final completer = Completer<ConnectionStateUpdate>();

    sub = _ble.connectToDevice(
      id: deviceId,
      connectionTimeout: timeout,
      servicesWithCharacteristicsToDiscover: {},
    ).listen((event) {
      if (event.connectionState == DeviceConnectionState.connected ||
          event.connectionState == DeviceConnectionState.disconnected) {
        completer.complete(event);
      }
    }, onError: (e) {
      if (!completer.isCompleted) completer.completeError(e);
    });

    final result = await completer.future;
    await sub.cancel();
    return result;
  }

  Stream<ConnectionStateUpdate> connectionStream(String deviceId) {
    return _ble.connectToDevice(id: deviceId, connectionTimeout: const Duration(seconds: 12));
  }

  Future<void> requestMtu(String deviceId, int mtu) async {
    try { await _ble.requestMtu(deviceId: deviceId, mtu: mtu); } catch (_) {}
  }

  Stream<List<int>> subscribe({required String deviceId, required Uuid service, required Uuid characteristic}) {
    final ch = QualifiedCharacteristic(serviceId: service, characteristicId: characteristic, deviceId: deviceId);
    return _ble.subscribeToCharacteristic(ch);
  }

  Future<void> write({required String deviceId, required Uuid service, required Uuid characteristic, required List<int> value, bool withResponse = true}) {
    final ch = QualifiedCharacteristic(serviceId: service, characteristicId: characteristic, deviceId: deviceId);
    return _ble.writeCharacteristicWithResponse(ch, value: value);
  }

  Future<List<int>> read({required String deviceId, required Uuid service, required Uuid characteristic}) {
    final ch = QualifiedCharacteristic(serviceId: service, characteristicId: characteristic, deviceId: deviceId);
    return _ble.readCharacteristic(ch);
  }
}
