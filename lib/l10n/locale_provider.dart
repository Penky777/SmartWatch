// lib/l10n/locale_provider.dart

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'app_localizations.dart';

/// Provider pre správu jazyka aplikácie
class LocaleProvider extends ChangeNotifier {
  static const String _prefsKey = 'app_language';

  AppLanguage _language = AppLanguage.sk; // default slovenčina

  AppLanguage get language => _language;

  LocaleProvider() {
    _loadLanguage();
  }

  /// Načítanie jazyka zo SharedPreferences
  Future<void> _loadLanguage() async {
    final prefs = await SharedPreferences.getInstance();
    final saved = prefs.getString(_prefsKey);

    if (saved != null) {
      _language = AppLanguage.values.firstWhere(
            (e) => e.name == saved,
        orElse: () => AppLanguage.sk,
      );
      notifyListeners();
    }
  }

  /// Zmena jazyka
  Future<void> setLanguage(AppLanguage lang) async {
    if (_language == lang) return;

    _language = lang;
    notifyListeners();

    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_prefsKey, lang.name);

    debugPrint('Language changed to: ${lang.name}');
  }

  /// Toggle medzi SK a EN
  Future<void> toggleLanguage() async {
    final newLang = _language == AppLanguage.sk ? AppLanguage.en : AppLanguage.sk;
    await setLanguage(newLang);
  }

  /// Či je aktuálne angličtina
  bool get isEnglish => _language == AppLanguage.en;

  /// Či je aktuálne slovenčina
  bool get isSlovak => _language == AppLanguage.sk;

  /// Názov aktuálneho jazyka
  String get languageName {
    switch (_language) {
      case AppLanguage.sk:
        return 'Slovenčina';
      case AppLanguage.en:
        return 'English';
    }
  }

  /// Skratka aktuálneho jazyka
  String get languageCode {
    switch (_language) {
      case AppLanguage.sk:
        return 'SK';
      case AppLanguage.en:
        return 'EN';
    }
  }
}