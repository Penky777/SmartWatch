import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../widgets/metric_chip.dart';
import '../test_ids.dart';

class ActivityScreen extends StatelessWidget {
  const ActivityScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Aktivita",
      titleKey: TKeys.titleActivity,
      subtitle: "Kroky, kalórie, vzdialenosť",
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.sync))],
      child: ListView(
        children: [
          SectionCard(
            title: "Dnes",
            child: Wrap(
              spacing: 8, runSpacing: 8,
              children: const [
                MetricChip(icon: Icons.directions_walk, label: "Kroky", value: "8 240"),
                MetricChip(icon: Icons.local_fire_department, label: "Kalórie", value: "540 kcal"),
                MetricChip(icon: Icons.straighten, label: "Vzdialenosť", value: "6.2 km"),
              ],
            ),
          ),
          SectionCard(
            title: "Týždeň",
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: const [
                Text("Priemer krokov: 7 950"),
                SizedBox(height: 6),
                LinearProgressIndicator(value: 0.79),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
