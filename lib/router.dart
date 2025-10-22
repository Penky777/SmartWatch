import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'screens/home_screen.dart';
import 'screens/health_screen.dart';
import 'screens/activity_screen.dart';
import 'screens/calendar_screen.dart';
import 'screens/weather_screen.dart';
import 'screens/notifications_screen.dart';
import 'screens/watchfaces_screen.dart';
import 'screens/settings_screen.dart';

final appRouter = GoRouter(routes: [
  GoRoute(path: '/', builder: (_, __) => const HomeScreen()),
  GoRoute(path: '/health', builder: (_, __) => const HealthScreen()),
  GoRoute(path: '/activity', builder: (_, __) => const ActivityScreen()),
  GoRoute(path: '/calendar', builder: (_, __) => const CalendarScreen()),
  GoRoute(path: '/weather', builder: (_, __) => const WeatherScreen()),
  GoRoute(path: '/notifications', builder: (_, __) => const NotificationsScreen()),
  GoRoute(path: '/watchfaces', builder: (_, __) => const WatchfacesScreen()),
  GoRoute(path: '/settings', builder: (_, __) => const SettingsScreen()),
]);
