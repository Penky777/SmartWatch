// lib/providers/provider_extensions.dart

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../l10n/locale_provider.dart';
import '../theme/theme_provider.dart';

/// Extension metódy na BuildContext pre jednoduchší prístup k providerom
extension ProviderExtensions on BuildContext {

  /// Získa LocaleProvider (s listen: true - prekresľuje sa pri zmene)
  LocaleProvider get localeProvider => Provider.of<LocaleProvider>(this);

  /// Získa ThemeProvider (s listen: true - prekresľuje sa pri zmene)
  ThemeProvider get themeProvider => Provider.of<ThemeProvider>(this);

  /// Získa LocaleProvider bez prekresľovania (listen: false)
  LocaleProvider get localeProviderRead => Provider.of<LocaleProvider>(this, listen: false);

  /// Získa ThemeProvider bez prekresľovania (listen: false)
  ThemeProvider get themeProviderRead => Provider.of<ThemeProvider>(this, listen: false);
}