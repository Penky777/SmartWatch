import 'package:flutter/material.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';

import 'theme/app_theme.dart';
import 'router.dart';
import 'di.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  setupDi();

  // ✅ Init pre foreground service (Android)
  FlutterForegroundTask.init(
    androidNotificationOptions: AndroidNotificationOptions(
      channelId: 'ble_channel',
      channelName: 'BLE Connection',
      channelDescription: 'Keeps smartwatch connected in background',
      channelImportance: NotificationChannelImportance.LOW,
      priority: NotificationPriority.LOW,
    ),
    foregroundTaskOptions: const ForegroundTaskOptions(
      interval: 5000,
      autoRunOnBoot: false,
      allowWakeLock: true,
      allowWifiLock: true,
    ), iosNotificationOptions: const IOSNotificationOptions(
      showNotification: true,
    ),
  );

  runApp(const SmartWatchApp());
}

class SmartWatchApp extends StatelessWidget {
  const SmartWatchApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SmartWatchApp',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.dark(),
      onGenerateRoute: generateRoute,
      initialRoute: AppRoutes.home,
    );
  }
}
