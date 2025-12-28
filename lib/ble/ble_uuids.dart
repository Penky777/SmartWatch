// lib/services/ble/ble_uuids.dart
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

class BleUUIDs {
  static final Uuid service =
  Uuid.parse('12345678-9abc-def0-1234-567890abcdef');

  // RX = phone -> watch (WRITE)
  static final Uuid rxChar =
  Uuid.parse('abcdef12-3456-789a-bcde-f01234567890');

  // TX = watch -> phone (NOTIFY)
  static final Uuid txChar =
  Uuid.parse('abcdef12-3456-789a-bcde-f01234567891');
}
