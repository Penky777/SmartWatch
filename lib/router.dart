// lib/router.dart

import 'package:flutter/material.dart';
import 'screens/home_screen.dart';
import 'screens/activity_screen.dart';
import 'screens/calendar_screen.dart';
import 'screens/health_screen.dart';
import 'screens/settings_screen.dart';
import 'screens/watchfaces_screen.dart';
import 'screens/weather_screen.dart';
import 'ble/ble_screen.dart';

class AppRoutes {
  static const home = '/';
  static const activity = '/activity';
  static const calendar = '/calendar';
  static const health = '/health';
  static const settings = '/settings';
  static const watchfaces = '/watchfaces';
  static const weather = '/weather';
  static const ble = '/ble';
}

Route<dynamic> generateRoute(RouteSettings settings) {
  switch (settings.name) {
    case AppRoutes.home:
      return _page(const HomeScreen());
    case AppRoutes.activity:
      return _page(const ActivityScreen());
    case AppRoutes.calendar:
      return _page(const CalendarScreen());
    case AppRoutes.health:
      return _page(const HealthScreen());
    case AppRoutes.settings:
      return _page(const SettingsScreen());
    case AppRoutes.watchfaces:
      return _page(const WatchfacesScreen());
    case AppRoutes.weather:
      return _page(const WeatherScreen());
    case AppRoutes.ble:
      return _page(const BleScreen());
    default:
      return _page(const HomeScreen());
  }
}

MaterialPageRoute _page(Widget child) => MaterialPageRoute(builder: (_) => child);