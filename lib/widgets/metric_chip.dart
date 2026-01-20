import 'package:flutter/material.dart';

class MetricChip extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  const MetricChip({super.key, required this.icon, required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    return Chip(
      label: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16),
          const SizedBox(width: 6),
          Text("$label: "),
          Text(value, style: const TextStyle(fontWeight: FontWeight.w700)),
        ],
      ),
      backgroundColor: cs.surface.withOpacity(0.75),
      side: BorderSide(color: cs.primary.withOpacity(0.35)),
    );
  }
}
