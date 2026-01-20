// lib/services/ble/ble_permissions.dart
import 'package:permission_handler/permission_handler.dart';

Future<bool> ensureBlePermissions() async {
  final requests = <Permission>[
    Permission.bluetoothScan,
    Permission.bluetoothConnect,
    // pre staršie Androidy treba aj location (scan vyžaduje)
    Permission.locationWhenInUse,
  ];
  final results = await requests.request();
  return results.values.every((s) => s.isGranted);
}
