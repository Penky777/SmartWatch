import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'ble_task_handler.dart';

Future<void> startBleService() async {
  final running = await FlutterForegroundTask.isRunningService;
  if (running) return;

  await FlutterForegroundTask.startService(
    notificationTitle: 'SmartWatch pripojené',
    notificationText: 'BLE spojenie beží na pozadí',
    callback: startCallback,
  );
}

Future<void> stopBleService() async {
  await FlutterForegroundTask.stopService();
}

@pragma('vm:entry-point')
void startCallback() {
  FlutterForegroundTask.setTaskHandler(BleTaskHandler());
}
