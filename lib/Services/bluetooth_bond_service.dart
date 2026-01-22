// lib/services/bluetooth_bond_service.dart
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

/// Service pre správu Bluetooth bondingu (Android only)
class BluetoothBondService {
  static const _channel = MethodChannel('com.example.smartwatch_app/bluetooth');

  /// Odstráni bonding pre dané zariadenie
  /// Vracia true ak bolo úspešné
  static Future<bool> removeBond(String deviceId) async {
    try {
      final result = await _channel.invokeMethod<bool>('removeBond', {
        'deviceId': deviceId,
      });
      debugPrint('BluetoothBondService: removeBond($deviceId) = $result');
      return result ?? false;
    } on PlatformException catch (e) {
      debugPrint('BluetoothBondService: removeBond failed: ${e.message}');
      return false;
    }
  }

  /// Získa zoznam spárovaných zariadení
  static Future<List<BondedDevice>> getBondedDevices() async {
    try {
      final result = await _channel.invokeMethod<List>('getBondedDevices');
      if (result == null) return [];

      return result.map((item) {
        final map = Map<String, String>.from(item);
        return BondedDevice(
          id: map['id'] ?? '',
          name: map['name'] ?? 'Unknown',
        );
      }).toList();
    } on PlatformException catch (e) {
      debugPrint('BluetoothBondService: getBondedDevices failed: ${e.message}');
      return [];
    }
  }

  /// Získa stav bondingu pre zariadenie
  /// 10 = BOND_NONE, 11 = BOND_BONDING, 12 = BOND_BONDED
  static Future<BondState> getBondState(String deviceId) async {
    try {
      final result = await _channel.invokeMethod<int>('getBondState', {
        'deviceId': deviceId,
      });

      switch (result) {
        case 10:
          return BondState.none;
        case 11:
          return BondState.bonding;
        case 12:
          return BondState.bonded;
        default:
          return BondState.none;
      }
    } on PlatformException catch (e) {
      debugPrint('BluetoothBondService: getBondState failed: ${e.message}');
      return BondState.none;
    }
  }
}

/// Stav bondingu
enum BondState {
  none,     // Nie je spárované
  bonding,  // Prebieha párovanie
  bonded,   // Spárované
}

/// Model pre spárované zariadenie
class BondedDevice {
  final String id;
  final String name;

  BondedDevice({required this.id, required this.name});

  @override
  String toString() => 'BondedDevice($name, $id)';
}