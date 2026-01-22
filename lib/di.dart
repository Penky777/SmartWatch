// lib/di.dart
import 'package:get_it/get_it.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import 'ble/ble_client.dart';
import 'ble/ble_repository.dart';

final getIt = GetIt.instance;

/// Nastavenie Dependency Injection
/// Teraz je async kvôli inicializácii Hive v BleRepository
Future<void> setupDi() async {
  // BLE stack
  getIt.registerLazySingleton<FlutterReactiveBle>(() => FlutterReactiveBle());
  getIt.registerLazySingleton<BleClient>(() => BleClient(getIt<FlutterReactiveBle>()));

  // BLE Repository
  final bleRepo = BleRepository(getIt<BleClient>());

  // Inicializuj Hive a načítaj históriu
  await bleRepo.initHive();

  getIt.registerSingleton<BleRepository>(bleRepo);
}