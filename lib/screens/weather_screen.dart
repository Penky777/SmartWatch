import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';

class WeatherScreen extends StatelessWidget {
  const WeatherScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Počasie",
      subtitle: "Trebišov, dnes",
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.my_location))],
      child: ListView(
        children: [
          SectionCard(
            title: "Aktuálne",
            child: Row(
              children: [
                const Icon(Icons.cloud, size: 34),
                const SizedBox(width: 12),
                const Text("Oblačno, 9°C"),
                const Spacer(),
                Text("Pocitovo 7°C", style: Theme.of(context).textTheme.bodySmall),
              ],
            ),
          ),
          SectionCard(
            title: "Ďalšie hodiny",
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: const [
                _HourBox(h: "12:00", t: "9°"),
                _HourBox(h: "15:00", t: "10°"),
                _HourBox(h: "18:00", t: "7°"),
                _HourBox(h: "21:00", t: "5°"),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _HourBox extends StatelessWidget {
  final String h, t;
  const _HourBox({required this.h, required this.t});
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(h, style: const TextStyle(fontWeight: FontWeight.w600)),
        const SizedBox(height: 6),
        const Icon(Icons.cloud_queue, size: 20),
        const SizedBox(height: 6),
        Text(t),
      ],
    );
  }
}
