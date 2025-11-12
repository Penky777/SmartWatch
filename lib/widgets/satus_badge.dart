import 'package:flutter/material.dart';
import '../theme/app_colors.dart';

class StatusBadge extends StatelessWidget {
  final String text;
  final Color color;
  const StatusBadge.connected({super.key})
      : text = "Pripojené",
        color = AppColors.success;
  const StatusBadge.connecting({super.key})
      : text = "Pripájanie…",
        color = AppColors.warning;
  const StatusBadge.disconnected({super.key})
      : text = "Odpojené",
        color = AppColors.danger;
  const StatusBadge.custom({super.key, required this.text, required this.color});

  @override
  Widget build(BuildContext context) {
    return Chip(
      label: Text(text),
      backgroundColor: color.withOpacity(0.16),
      labelStyle: const TextStyle(fontWeight: FontWeight.w600),
      side: BorderSide(color: color.withOpacity(0.55)),
    );
  }
}
