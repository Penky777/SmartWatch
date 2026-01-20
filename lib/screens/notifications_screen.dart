import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../test_ids.dart';

class NotificationsScreen extends StatelessWidget {
  const NotificationsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Notifikácie",
      titleKey: TKeys.titleNotifications,
      subtitle: "Posledné upozornenia",
      actions: [IconButton(onPressed: () {}, icon: const Icon(Icons.clear_all))],
      child: ListView(
        children: [
          SectionCard(
            title: "Dnes",
            child: Column(
              children: const [
                _NotifRow(icon: Icons.message, title: "Messenger", body: "Skúška 18:30 platí?"),
                _NotifRow(icon: Icons.mail, title: "Gmail", body: "FRI: Zadanie odovzdané"),
                _NotifRow(icon: Icons.sports_handball, title: "ŠK Zemplín", body: "Zápas 13.11. 14:30"),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _NotifRow extends StatelessWidget {
  final IconData icon; final String title; final String body;
  const _NotifRow({required this.icon, required this.title, required this.body});
  @override
  Widget build(BuildContext context) {
    return ListTile(
      leading: Icon(icon),
      title: Text(title, style: const TextStyle(fontWeight: FontWeight.w700)),
      subtitle: Text(body),
      trailing: IconButton(icon: const Icon(Icons.delete_outline), onPressed: () {}),
    );
  }
}
