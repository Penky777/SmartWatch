import 'dart:async';

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../test_ids.dart';
import '../services/notifications/android_notif_stream.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});
  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen>
    with WidgetsBindingObserver {
  static const _prefsKeyNotif = 'notif_forward_enabled';

  bool notifications = false; // ✅ default OFF
  bool darkMode = true;
  bool autoSync = true;

  final _notif = AndroidNotifStream();

  // ✅ DEBUG subscription na notifikácie (z Android EventChannel)
  StreamSubscription<Map<String, dynamic>>? _notifSub;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);

    _loadNotifState();

    // ✅ DEBUG: uvidíš v logu všetky notifikácie ktoré prídu do Flutteru
    _notifSub = _notif.stream.listen(
          (m) => debugPrint('NOTIF EVENT: $m'),
      onError: (e) => debugPrint('NOTIF ERROR: $e'),
    );
  }

  @override
  void dispose() {
    _notifSub?.cancel();
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  // keď sa vrátiš zo Settings (resume), prekontroluj reálny stav
  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _syncWithSystemPermission();
    }
  }

  Future<void> _loadNotifState() async {
    final prefs = await SharedPreferences.getInstance();
    final saved = prefs.getBool(_prefsKeyNotif) ?? false; // ✅ default false

    setState(() => notifications = saved);

    // ak user mal zapnuté, ale systémové povolenie nie je, vypni
    await _syncWithSystemPermission();
  }

  Future<void> _syncWithSystemPermission() async {
    try {
      final systemEnabled = await _notif.isAccessEnabled();
      debugPrint("SYSTEM notif access enabled = $systemEnabled");

      if (!mounted) return;

      // ak systém nepovolil, tak switch musí byť false
      if (!systemEnabled && notifications) {
        setState(() => notifications = false);
        final prefs = await SharedPreferences.getInstance();
        await prefs.setBool(_prefsKeyNotif, false);
      }
    } catch (e) {
      debugPrint("isAccessEnabled FAILED: $e");
    }
  }

  Future<void> _toggleNotifications(bool v) async {
    if (!v) {
      // user vypol -> len uložiť, systémové povolenie nezoberieme
      setState(() => notifications = false);
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool(_prefsKeyNotif, false);
      return;
    }

    // user zapína -> otvor settings a po návrate sa to samo zosyncuje
    await _notif.openAccessSettings();

    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('V nastaveniach povoľ prístup k notifikáciám pre túto appku.'),
      ),
    );

    // nech switch neklame pred povolením
    setState(() => notifications = false);
  }

  // keď systémové povolenie je ON, tak až vtedy si uložíme true
  Future<void> _applyTrueIfAllowed() async {
    try {
      final ok = await _notif.isAccessEnabled();
      debugPrint("SYSTEM notif access (apply) = $ok");

      if (!mounted) return;
      if (ok) {
        setState(() => notifications = true);
        final prefs = await SharedPreferences.getInstance();
        await prefs.setBool(_prefsKeyNotif, true);
      }
    } catch (e) {
      debugPrint("applyTrueIfAllowed FAILED: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    return ScreenScaffold(
      title: "Nastavenia",
      titleKey: TKeys.titleSettings,
      subtitle: "Aplikácia a hodinky",
      child: ListView(
        children: [
          SectionCard(
            title: "Aplikácia",
            child: Column(
              children: [
                SwitchListTile(
                  value: darkMode,
                  onChanged: (v) => setState(() => darkMode = v),
                  title: const Text("Tmavý režim"),
                ),
                SwitchListTile(
                  value: notifications,
                  onChanged: (v) async {
                    await _toggleNotifications(v);
                    // po návrate zo settings sa to zosyncuje,
                    // ale keď user povolí rýchlo a vráti sa, toto to hneď aplikuje:
                    await _applyTrueIfAllowed();
                  },
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
                  value: autoSync,
                  onChanged: (v) => setState(() => autoSync = v),
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
