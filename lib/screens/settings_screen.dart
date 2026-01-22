// lib/screens/settings_screen.dart

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../test_ids.dart';
import '../services/notifications/android_notif_stream.dart';
import '../l10n/app_localizations.dart';
import '../providers/provider_extensions.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen>
    with WidgetsBindingObserver {
  static const _prefsKeyNotif = 'notif_forward_enabled';

  bool notifications = false;
  bool autoSync = true;

  final _notif = AndroidNotifStream();
  StreamSubscription<Map<String, dynamic>>? _notifSub;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _loadNotifState();

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

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _syncWithSystemPermission();
    }
  }

  Future<void> _loadNotifState() async {
    final prefs = await SharedPreferences.getInstance();
    final saved = prefs.getBool(_prefsKeyNotif) ?? false;

    setState(() => notifications = saved);
    await _syncWithSystemPermission();
  }

  Future<void> _syncWithSystemPermission() async {
    try {
      final systemEnabled = await _notif.isAccessEnabled();
      debugPrint("SYSTEM notif access enabled = $systemEnabled");

      if (!mounted) return;

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
      setState(() => notifications = false);
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool(_prefsKeyNotif, false);
      return;
    }

    await _notif.openAccessSettings();

    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(context.tr('enable_notif_access')),
      ),
    );

    setState(() => notifications = false);
  }

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

  void _showLanguageDialog() {
    // ✅ DÔLEŽITÉ: Použiť Read verzie v event handleroch!
    final localeProvider = context.localeProviderRead;
    final currentLang = localeProvider.language;
    final l10n = AppLocalizations(currentLang);

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(l10n.tr('language')),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              leading: Radio<AppLanguage>(
                value: AppLanguage.sk,
                groupValue: currentLang,
                onChanged: (val) {
                  if (val != null) {
                    localeProvider.setLanguage(val);
                    Navigator.of(ctx).pop();
                  }
                },
              ),
              title: Text('🇸🇰 ${l10n.tr('language_slovak')}'),
              onTap: () {
                localeProvider.setLanguage(AppLanguage.sk);
                Navigator.of(ctx).pop();
              },
            ),
            ListTile(
              leading: Radio<AppLanguage>(
                value: AppLanguage.en,
                groupValue: currentLang,
                onChanged: (val) {
                  if (val != null) {
                    localeProvider.setLanguage(val);
                    Navigator.of(ctx).pop();
                  }
                },
              ),
              title: Text('🇬🇧 ${l10n.tr('language_english')}'),
              onTap: () {
                localeProvider.setLanguage(AppLanguage.en);
                Navigator.of(ctx).pop();
              },
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(ctx).pop(),
            child: Text(l10n.tr('cancel')),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    // ✅ V build() môžeš použiť normálne verzie (s listen: true)
    final l10n = context.l10n;
    final localeProvider = context.localeProvider;
    final themeProvider = context.themeProvider;

    return ScreenScaffold(
      title: l10n.tr('settings_title'),
      titleKey: TKeys.titleSettings,
      subtitle: l10n.tr('settings_subtitle'),
      child: ListView(
        children: [
          // Sekcia Aplikácia
          SectionCard(
            title: l10n.tr('app_section'),
            child: Column(
              children: [
                // Dark mode prepínač
                SwitchListTile(
                  value: themeProvider.isDarkMode,
                  onChanged: (v) {
                    // ✅ Tu je OK použiť themeProvider z build()
                    // lebo onChanged callback dostane hodnotu priamo
                    context.themeProviderRead.setDarkMode(v);
                  },
                  title: Text(l10n.tr('dark_mode')),
                  secondary: Icon(
                    themeProvider.isDarkMode ? Icons.dark_mode : Icons.light_mode,
                  ),
                ),

                // Systémové notifikácie
                SwitchListTile(
                  value: notifications,
                  onChanged: (v) async {
                    await _toggleNotifications(v);
                    await _applyTrueIfAllowed();
                  },
                  title: Text(l10n.tr('system_notifications')),
                  secondary: const Icon(Icons.notifications),
                ),

                // Jazyk
                ListTile(
                  leading: const Icon(Icons.language),
                  title: Text(l10n.tr('language')),
                  subtitle: Text(localeProvider.languageName),
                  trailing: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Container(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 8,
                          vertical: 4,
                        ),
                        decoration: BoxDecoration(
                          color: Theme.of(context).colorScheme.primary.withOpacity(0.2),
                          borderRadius: BorderRadius.circular(6),
                        ),
                        child: Text(
                          localeProvider.languageCode,
                          style: TextStyle(
                            fontWeight: FontWeight.bold,
                            color: Theme.of(context).colorScheme.primary,
                          ),
                        ),
                      ),
                      const SizedBox(width: 8),
                      const Icon(Icons.chevron_right),
                    ],
                  ),
                  onTap: _showLanguageDialog,
                ),
              ],
            ),
          ),

          // Sekcia Synchronizácia
          // SectionCard(
          //   title: l10n.tr('sync_section'),
          //   child: Column(
          //     children: [
          //       SwitchListTile(
          //         value: autoSync,
          //         onChanged: (v) => setState(() => autoSync = v),
          //         title: Text(l10n.tr('auto_sync')),
          //         secondary: const Icon(Icons.sync),
          //       ),
          //       ListTile(
          //         leading: const Icon(Icons.sync),
          //         title: Text(l10n.tr('manual_sync')),
          //         trailing: const Icon(Icons.chevron_right),
          //         onTap: () {},
          //       ),
          //     ],
          //   ),
          // ),
        ],
      ),
    );
  }
}