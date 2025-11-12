import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';

class CalendarScreen extends StatelessWidget {
  const CalendarScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Kalendár",
      subtitle: "Najbližšie udalosti",
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.add))],
      child: ListView(
        children: [
          SectionCard(
            title: "Dnes",
            child: Column(
              children: const [
                _EventRow(time: "14:30", title: "Mladší A vs. Mladší B", place: "ŠH Trebišov"),
                _EventRow(time: "18:30", title: "Tréning mužov", place: "ŠH Trebišov"),
              ],
            ),
          ),
          SectionCard(
            title: "Zajtra",
            child: Column(
              children: const [
                _EventRow(time: "09:00", title: "Bežecká príprava", place: "Zemplínska šírava"),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _EventRow extends StatelessWidget {
  final String time, title, place;
  const _EventRow({required this.time, required this.title, required this.place});

  @override
  Widget build(BuildContext context) {
    final th = Theme.of(context).textTheme;
    return ListTile(
      contentPadding: EdgeInsets.zero,
      leading: Text(time, style: th.titleMedium?.copyWith(fontWeight: FontWeight.bold)),
      title: Text(title),
      subtitle: Text(place),
      trailing: const Icon(Icons.chevron_right),
      onTap: () {},
    );
  }
}
