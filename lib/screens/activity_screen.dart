// lib/screens/activity_screen.dart

import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../widgets/metric_chip.dart';
import '../test_ids.dart';
import '../l10n/app_localizations.dart';

class ActivityScreen extends StatelessWidget {
  const ActivityScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = context.l10n;

    return ScreenScaffold(
      title: l10n.tr('activity_title'),
      titleKey: TKeys.titleActivity,
      subtitle: l10n.tr('activity_subtitle'),
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.sync))],
      child: ListView(
        children: [
          SectionCard(
            title: l10n.tr('today'),
            child: Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                MetricChip(
                  icon: Icons.directions_walk,
                  label: l10n.tr('steps'),
                  value: "8 240",
                ),
                MetricChip(
                  icon: Icons.local_fire_department,
                  label: l10n.tr('calories'),
                  value: "540 kcal",
                ),
                MetricChip(
                  icon: Icons.straighten,
                  label: l10n.tr('distance'),
                  value: "6.2 km",
                ),
              ],
            ),
          ),
          SectionCard(
            title: l10n.tr('week'),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text("${l10n.tr('average_steps')}: 7 950"),
                const SizedBox(height: 6),
                const LinearProgressIndicator(value: 0.79),
              ],
            ),
          ),
        ],
      ),
    );
  }
}