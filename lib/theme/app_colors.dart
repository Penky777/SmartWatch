// lib/theme/app_colors.dart

import 'package:flutter/material.dart';

/// Klubové/brand farby – modrá/biela/antracit
class AppColors {
  // Primárne farby
  static const Color primary = Color(0xFF1E63F0); // živá modrá
  static const Color onPrimary = Colors.white;
  static const Color secondary = Color(0xFF00C2FF); // akcent (neónovo-modrastá)

  // Dark mode farby
  static const Color backgroundDark = Color(0xFF121317); // antracit / "ink"
  static const Color surfaceDark = Color(0xFF1A1B21);

  // Light mode farby
  static const Color backgroundLight = Color(0xFFD0D0D8); // tmavšia sivá - dobre viditeľná
  static const Color surfaceLight = Color(0xFFFFFFFF); // biela pre karty

  // Stavové farby
  static const Color success = Color(0xFF2ECC71);
  static const Color warning = Color(0xFFF1C40F);
  static const Color danger = Color(0xFFE74C3C);
}