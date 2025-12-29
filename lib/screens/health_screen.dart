import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../widgets/metric_chip.dart';
import '../test_ids.dart';

class HealthScreen extends StatelessWidget {
  const HealthScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Zdravie",
      titleKey: TKeys.titleHealth,
      subtitle: "Tep, SpO₂, stres",
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.refresh))],
      child: ListView(
        children: [
          SectionCard(
            title: "Realtime",
            child: Wrap(
              spacing: 8, runSpacing: 8,
              children: const [
                MetricChip(icon: Icons.favorite, label: "Tep", value: "72 bpm"),
                MetricChip(icon: Icons.health_and_safety, label: "SpO₂", value: "98%"),
                MetricChip(icon: Icons.self_improvement, label: "Stres", value: "nízky"),
              ],
            ),
          ),
          SectionCard(
            title: "Trend (24h)",
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: const [
                Text("🡻 minimum: 55 bpm   🡹 maximum: 138 bpm"),
                SizedBox(height: 8),
                LinearProgressIndicator(value: 0.55),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
