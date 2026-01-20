import 'dart:async';
import 'dart:isolate';

import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../di.dart';
import 'ble_repository.dart';

class BleTaskHandler extends TaskHandler {
  BleRepository? _repo;
  Timer? _timer;

  @override
  Future<void> onStart(DateTime timestamp, SendPort? sendPort) async {
    setupDi();
    _repo = getIt<BleRepository>();

    final prefs = await SharedPreferences.getInstance();
    final lastId = prefs.getString('ble_last_device');

    if (lastId != null && lastId.isNotEmpty) {
      _repo!.connect(lastId);
    }

    _timer = Timer.periodic(const Duration(seconds: 10), (_) async {
      final repo = _repo;
      if (repo == null) return;

      final id = repo.deviceId;
      if (id == null) return;

      if (repo.currentStatus != BleStatus.connected &&
          repo.currentStatus != BleStatus.connecting) {
        repo.connect(id);
      }
    });
  }

  @override
  Future<void> onEvent(DateTime timestamp, SendPort? sendPort) async {
    // nič netreba
  }

  @override
  Future<void> onDestroy(DateTime timestamp, SendPort? sendPort) async {
    _timer?.cancel();
    _timer = null;
  }

  @override
  void onRepeatEvent(DateTime timestamp, SendPort? sendPort) {
    // nič netreba
  }
}
