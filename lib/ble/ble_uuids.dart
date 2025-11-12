

// lib/services/ble/ble_uuids.dart
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart' show Uuid;

class BleUUIDs {
  static final Uuid service = Uuid.parse("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  static final Uuid txChar  = Uuid.parse("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
  static final Uuid rxChar  = Uuid.parse("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
}