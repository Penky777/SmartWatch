// lib/l10n/app_localizations.dart

import 'package:flutter/material.dart';

/// Podporované jazyky
enum AppLanguage { sk, en }

/// Hlavná trieda pre preklady
class AppLocalizations {
  final AppLanguage language;

  AppLocalizations(this.language);

  /// Získanie inštancie z kontextu
  static AppLocalizations of(BuildContext context) {
    return context.dependOnInheritedWidgetOfExactType<LocalizationsProvider>()!.localizations;
  }

  /// Skratka pre preklad
  String tr(String key) => _translations[language]?[key] ?? key;

  /// Všetky preklady
  static final Map<AppLanguage, Map<String, String>> _translations = {
    // ==================== SLOVENČINA ====================
    AppLanguage.sk: {
      // Všeobecné
      'app_title': 'SmartWatch',
      'loading': 'Načítavam...',
      'error': 'Chyba',
      'cancel': 'Zrušiť',
      'ok': 'OK',
      'save': 'Uložiť',
      'delete': 'Zmazať',
      'search': 'Hľadať',
      'stop': 'Stop',
      'disconnect': 'Odpojiť',
      'connected': 'Pripojené',
      'disconnected': 'Odpojené',
      'connecting': 'Pripájam...',
      'scanning': 'Skenujem...',

      // Home Screen
      'home_title': 'SmartWatch',
      'home_subtitle': 'Všetko dôležité na jednom mieste',
      'quick_access': 'Rýchly prístup',
      'today_summary': 'Dnešné zhrnutie',

      // Navigation tiles
      'tile_activity': 'Aktivita',
      'tile_health': 'Zdravie',
      'tile_weather': 'Počasie',
      'tile_calendar': 'Kalendár',
      'tile_notifications': 'Notifikácie',
      'tile_watchfaces': 'Watchfaces',
      'tile_settings': 'Nastavenia',
      'tile_bluetooth': 'Bluetooth',

      // Metrics
      'steps': 'Kroky',
      'calories': 'Kalórie',
      'distance': 'Vzdialenosť',
      'heart_rate': 'Tep',
      'spo2': 'SpO₂',
      'stress': 'Stres',

      // Activity Screen
      'activity_title': 'Aktivita',
      'activity_subtitle': 'Kroky, kalórie, vzdialenosť',
      'today': 'Dnes',
      'week': 'Týždeň',
      'average_steps': 'Priemer krokov',

      // Health Screen
      'health_title': 'Zdravie',
      'realtime': 'Realtime',
      'heart_rate_history': 'Tep - história',
      'spo2_history': 'SpO₂ - história',
      'trend_session': 'Trend (session)',
      'waiting_for_data': 'Čakám na dáta z hodiniek...',
      'watch_not_connected': 'Hodinky nie sú pripojené',
      'min': 'min',
      'max': 'max',
      'measurements': 'Meraní',
      'stress_low': 'nízky',
      'stress_normal': 'normálny',
      'stress_elevated': 'mierne zvýšený',
      'stress_high': 'vysoký',

      // Calendar Screen
      'calendar_title': 'Kalendár',
      'calendar_subtitle': 'Najbližšie udalosti',
      'tomorrow': 'Zajtra',

      // Weather Screen
      'weather_title': 'Počasie',
      'current': 'Aktuálne',
      'forecast': 'Predpoveď',
      'loading_weather': 'Načítavam počasie…',
      'weather_unavailable': 'Počasie nie je k dispozícii',
      'forecast_unavailable': 'Predpoveď nie je k dispozícii',
      'feels_like': 'Pocitovo',

      // Notifications Screen
      'notifications_title': 'Notifikácie',
      'notifications_subtitle': 'Posledné upozornenia',

      // Watchfaces Screen
      'watchfaces_title': 'Watchfaces',
      'watchfaces_subtitle': 'Vyber si vzhľad hodiniek',
      'collection': 'Kolekcia',

      // Settings Screen
      'settings_title': 'Nastavenia',
      'settings_subtitle': 'Aplikácia a hodinky',
      'app_section': 'Aplikácia',
      'dark_mode': 'Tmavý režim',
      'system_notifications': 'Systémové notifikácie',
      'language': 'Jazyk',
      'language_slovak': 'Slovenčina',
      'language_english': 'English',
      'sync_section': 'Synchronizácia',
      'auto_sync': 'Automatická synchronizácia',
      'manual_sync': 'Manuálna synchronizácia',
      'enable_notif_access': 'V nastaveniach povoľ prístup k notifikáciám pre túto appku.',

      // BLE Screen
      'ble_title': 'BLE hodinky',
      'paired': 'Spárované',
      'not_paired': 'Nespárované',
      'no_devices': 'Žiadne zariadenia.',
      'saved_device': 'Uložené zariadenie (nie je v dosahu)',
      'saved': 'Uložené',
      'rssi': 'RSSI',
      'ble_permissions_denied': 'BLE/Location povolenia neboli udelené. Povoľte ich v nastaveniach.',
      'pin': 'PIN',
      'message_placeholder': 'Správa pre hodinky…',
      'connect_first': 'Najprv sa pripoj...',
      'send': 'Poslať',
    },

    // ==================== ENGLISH ====================
    AppLanguage.en: {
      // General
      'app_title': 'SmartWatch',
      'loading': 'Loading...',
      'error': 'Error',
      'cancel': 'Cancel',
      'ok': 'OK',
      'save': 'Save',
      'delete': 'Delete',
      'search': 'Search',
      'stop': 'Stop',
      'disconnect': 'Disconnect',
      'connected': 'Connected',
      'disconnected': 'Disconnected',
      'connecting': 'Connecting...',
      'scanning': 'Scanning...',

      // Home Screen
      'home_title': 'SmartWatch',
      'home_subtitle': 'Everything important in one place',
      'quick_access': 'Quick Access',
      'today_summary': 'Today\'s Summary',

      // Navigation tiles
      'tile_activity': 'Activity',
      'tile_health': 'Health',
      'tile_weather': 'Weather',
      'tile_calendar': 'Calendar',
      'tile_notifications': 'Notifications',
      'tile_watchfaces': 'Watchfaces',
      'tile_settings': 'Settings',
      'tile_bluetooth': 'Bluetooth',

      // Metrics
      'steps': 'Steps',
      'calories': 'Calories',
      'distance': 'Distance',
      'heart_rate': 'Heart Rate',
      'spo2': 'SpO₂',
      'stress': 'Stress',

      // Activity Screen
      'activity_title': 'Activity',
      'activity_subtitle': 'Steps, calories, distance',
      'today': 'Today',
      'week': 'Week',
      'average_steps': 'Average steps',

      // Health Screen
      'health_title': 'Health',
      'realtime': 'Realtime',
      'heart_rate_history': 'Heart Rate - History',
      'spo2_history': 'SpO₂ - History',
      'trend_session': 'Trend (session)',
      'waiting_for_data': 'Waiting for watch data...',
      'watch_not_connected': 'Watch not connected',
      'min': 'min',
      'max': 'max',
      'measurements': 'Measurements',
      'stress_low': 'low',
      'stress_normal': 'normal',
      'stress_elevated': 'slightly elevated',
      'stress_high': 'high',

      // Calendar Screen
      'calendar_title': 'Calendar',
      'calendar_subtitle': 'Upcoming events',
      'tomorrow': 'Tomorrow',

      // Weather Screen
      'weather_title': 'Weather',
      'current': 'Current',
      'forecast': 'Forecast',
      'loading_weather': 'Loading weather…',
      'weather_unavailable': 'Weather not available',
      'forecast_unavailable': 'Forecast not available',
      'feels_like': 'Feels like',

      // Notifications Screen
      'notifications_title': 'Notifications',
      'notifications_subtitle': 'Recent alerts',

      // Watchfaces Screen
      'watchfaces_title': 'Watchfaces',
      'watchfaces_subtitle': 'Choose your watch style',
      'collection': 'Collection',

      // Settings Screen
      'settings_title': 'Settings',
      'settings_subtitle': 'App and watch',
      'app_section': 'Application',
      'dark_mode': 'Dark Mode',
      'system_notifications': 'System Notifications',
      'language': 'Language',
      'language_slovak': 'Slovenčina',
      'language_english': 'English',
      'sync_section': 'Synchronization',
      'auto_sync': 'Auto Sync',
      'manual_sync': 'Manual Sync',
      'enable_notif_access': 'Enable notification access for this app in settings.',

      // BLE Screen
      'ble_title': 'BLE Watch',
      'paired': 'Paired',
      'not_paired': 'Not Paired',
      'no_devices': 'No devices found.',
      'saved_device': 'Saved device (not in range)',
      'saved': 'Saved',
      'rssi': 'RSSI',
      'ble_permissions_denied': 'BLE/Location permissions not granted. Please enable in settings.',
      'pin': 'PIN',
      'message_placeholder': 'Message for watch…',
      'connect_first': 'Connect first...',
      'send': 'Send',
    },
  };
}

/// InheritedWidget pre distribúciu lokalizácií
class LocalizationsProvider extends InheritedWidget {
  final AppLocalizations localizations;

  const LocalizationsProvider({
    super.key,
    required this.localizations,
    required super.child,
  });

  @override
  bool updateShouldNotify(LocalizationsProvider oldWidget) {
    return localizations.language != oldWidget.localizations.language;
  }
}

/// Extension pre ľahší prístup
extension LocalizationsExtension on BuildContext {
  AppLocalizations get l10n => AppLocalizations.of(this);
  String tr(String key) => l10n.tr(key);
}