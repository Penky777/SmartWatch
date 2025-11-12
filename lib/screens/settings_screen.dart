import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});
  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  bool notifications = true;
  bool darkMode = true;
  bool autoSync = true;

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Nastavenia",
      subtitle: "Aplikácia a hodinky",
      child: ListView(
        children: [
          SectionCard(
            title: "Aplikácia",
            child: Column(
              children: [
                SwitchListTile(
                  value: darkMode, onChanged: (v) => setState(() => darkMode = v),
                  title: const Text("Tmavý režim"),
                ),
                SwitchListTile(
                  value: notifications, onChanged: (v) => setState(() => notifications = v),
                  title: const Text("Systémové notifikácie"),
                ),
              ],
            ),
          ),
          SectionCard(
            title: "Synchronizácia",
            child: Column(
              children: [
                SwitchListTile(
                  value: autoSync, onChanged: (v) => setState(() => autoSync = v),
                  title: const Text("Automatická synchronizácia"),
                ),
                ListTile(
                  leading: const Icon(Icons.sync),
                  title: const Text("Manuálna synchronizácia"),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () {},
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
