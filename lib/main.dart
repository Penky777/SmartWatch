// lib/main.dart

import 'package:flutter/material.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';

import 'theme/app_theme.dart';
import 'theme/theme_provider.dart';
import 'router.dart';
import 'di.dart';
import 'l10n/app_localizations.dart';
import 'l10n/locale_provider.dart';

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
    ),
    iosNotificationOptions: const IOSNotificationOptions(
      showNotification: true,
    ),
  );

  runApp(const SmartWatchApp());
}

class SmartWatchApp extends StatefulWidget {
  const SmartWatchApp({super.key});

  @override
  State<SmartWatchApp> createState() => _SmartWatchAppState();
}

class _SmartWatchAppState extends State<SmartWatchApp> {
  final LocaleProvider _localeProvider = LocaleProvider();
  final ThemeProvider _themeProvider = ThemeProvider();

  @override
  void initState() {
    super.initState();
    _localeProvider.addListener(_onSettingsChanged);
    _themeProvider.addListener(_onSettingsChanged);
  }

  @override
  void dispose() {
    _localeProvider.removeListener(_onSettingsChanged);
    _themeProvider.removeListener(_onSettingsChanged);
    _localeProvider.dispose();
    _themeProvider.dispose();
    super.dispose();
  }

  void _onSettingsChanged() {
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    return ThemeProviderScope(
      provider: _themeProvider,
      child: LocaleProviderScope(
        provider: _localeProvider,
        child: LocalizationsProvider(
          localizations: AppLocalizations(_localeProvider.language),
          child: MaterialApp(
            title: 'SmartWatchApp',
            debugShowCheckedModeBanner: false,
            theme: _themeProvider.isDarkMode ? AppTheme.dark() : AppTheme.light(),
            onGenerateRoute: generateRoute,
            initialRoute: AppRoutes.home,
          ),
        ),
      ),
    );
  }
}

/// InheritedWidget pre prístup k LocaleProvider
class LocaleProviderScope extends InheritedWidget {
  final LocaleProvider provider;

  const LocaleProviderScope({
    super.key,
    required this.provider,
    required super.child,
  });

  static LocaleProvider of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<LocaleProviderScope>();
    return scope!.provider;
  }

  @override
  bool updateShouldNotify(LocaleProviderScope oldWidget) {
    return provider != oldWidget.provider;
  }
}

/// InheritedWidget pre prístup k ThemeProvider
class ThemeProviderScope extends InheritedWidget {
  final ThemeProvider provider;

  const ThemeProviderScope({
    super.key,
    required this.provider,
    required super.child,
  });

  static ThemeProvider of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<ThemeProviderScope>();
    return scope!.provider;
  }

  @override
  bool updateShouldNotify(ThemeProviderScope oldWidget) {
    return provider != oldWidget.provider;
  }
}

/// Extension pre jednoduchý prístup k providerom
extension LocaleProviderExtension on BuildContext {
  LocaleProvider get localeProvider => LocaleProviderScope.of(this);
}

extension ThemeProviderExtension on BuildContext {
  ThemeProvider get themeProvider => ThemeProviderScope.of(this);
}