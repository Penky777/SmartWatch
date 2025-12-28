// lib/services/ble/ble_client.dart
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

class BleClient {
  final FlutterReactiveBle _ble;
  BleClient(this._ble);

  Stream<DiscoveredDevice> scanForDevices({Uuid? service}) {
    return _ble.scanForDevices(
      withServices: service != null ? [service] : const [],
      scanMode: ScanMode.lowLatency,
    );
  }

  Stream<ConnectionStateUpdate> connectionStream(String deviceId) {
    return _ble.connectToDevice(
      id: deviceId,
      connectionTimeout: const Duration(seconds: 10),
    );
  }

  Future<int?> requestMtu(String deviceId, int mtu) async {
    // Na niektorých platformách môže hodiť exception alebo vrátiť negotiated value.
    return _ble.requestMtu(deviceId: deviceId, mtu: mtu);
  }

  Stream<List<int>> subscribe({
    required String deviceId,
    required Uuid service,
    required Uuid characteristic,
  }) {
    final q = QualifiedCharacteristic(
      deviceId: deviceId,
      serviceId: service,
      characteristicId: characteristic,
    );
    return _ble.subscribeToCharacteristic(q);
  }

  Future<void> write({
    required String deviceId,
    required Uuid service,
    required Uuid characteristic,
    required List<int> value,
    bool withResponse = true,
  }) async {
    final q = QualifiedCharacteristic(
      deviceId: deviceId,
      serviceId: service,
      characteristicId: characteristic,
    );
    if (withResponse) {
      await _ble.writeCharacteristicWithResponse(q, value: value);
    } else {
      await _ble.writeCharacteristicWithoutResponse(q, value: value);
    }
  }
}
