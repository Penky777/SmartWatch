// lib/di.dart
import 'package:get_it/get_it.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import 'ble/ble_client.dart';
import 'ble/ble_repository.dart';

final getIt = GetIt.instance;

void setupDi() {
  // BLE stack
  getIt.registerLazySingleton<FlutterReactiveBle>(() => FlutterReactiveBle());
  getIt.registerLazySingleton<BleClient>(() => BleClient(getIt<FlutterReactiveBle>()));
  getIt.registerLazySingleton<BleRepository>(() => BleRepository(getIt<BleClient>()));
}
