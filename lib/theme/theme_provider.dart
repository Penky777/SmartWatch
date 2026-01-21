// lib/theme/theme_provider.dart

import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// Provider pre správu témy aplikácie (dark/light mode)
class ThemeProvider extends ChangeNotifier {
  static const String _prefsKey = 'app_dark_mode';

  bool _isDarkMode = true;

  bool get isDarkMode => _isDarkMode;

  ThemeProvider() {
    _loadTheme();
  }

  /// Načítanie témy zo SharedPreferences
  /// Pri prvom spustení použije systémovú tému
  Future<void> _loadTheme() async {
    final prefs = await SharedPreferences.getInstance();

    // Skontroluj či už máme uloženú hodnotu
    if (prefs.containsKey(_prefsKey)) {
      // ✅ Použij uloženú hodnotu
      _isDarkMode = prefs.getBool(_prefsKey) ?? true;
    } else {
      // ✅ Prvé spustenie - použij systémovú tému
      final brightness = SchedulerBinding.instance.platformDispatcher.platformBrightness;
      _isDarkMode = brightness == Brightness.dark;

      // Ulož túto hodnotu
      await prefs.setBool(_prefsKey, _isDarkMode);
      debugPrint('First run - using system theme: ${_isDarkMode ? "dark" : "light"}');
    }

    notifyListeners();
  }

  /// Nastavenie témy
  Future<void> setDarkMode(bool isDark) async {
    if (_isDarkMode == isDark) return;

    _isDarkMode = isDark;
    notifyListeners();

    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_prefsKey, isDark);

    debugPrint('Theme changed to: ${isDark ? "dark" : "light"}');
  }

  /// Toggle medzi dark a light
  Future<void> toggleTheme() async {
    await setDarkMode(!_isDarkMode);
  }
}