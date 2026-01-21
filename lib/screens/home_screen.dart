// lib/screens/home_screen.dart

import 'dart:async';

import 'package:flutter/material.dart';

import '../di.dart';
import '../ble/ble_repository.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../router.dart';
import '../test_ids.dart';
import '../l10n/app_localizations.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  late final BleRepository _repo;
  StreamSubscription<HealthData?>? _healthSub;

  @override
  void initState() {
    super.initState();
    _repo = getIt<BleRepository>();

    _healthSub = _repo.healthDataStream.listen((_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _healthSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = context.l10n;
    final h = _repo.healthData;

    final items = [
      _HomeItem(Icons.directions_run, l10n.tr('tile_activity'), AppRoutes.activity, TKeys.tileActivity),
      _HomeItem(Icons.favorite, l10n.tr('tile_health'), AppRoutes.health, TKeys.tileHealth),
      _HomeItem(Icons.cloud, l10n.tr('tile_weather'), AppRoutes.weather, TKeys.tileWeather),
      _HomeItem(Icons.calendar_month, l10n.tr('tile_calendar'), AppRoutes.calendar, TKeys.tileCalendar),
      _HomeItem(Icons.watch, l10n.tr('tile_watchfaces'), AppRoutes.watchfaces, TKeys.tileWatchfaces),
      _HomeItem(Icons.settings, l10n.tr('tile_settings'), AppRoutes.settings, TKeys.tileSettings),
      _HomeItem(Icons.bluetooth, l10n.tr('tile_bluetooth'), AppRoutes.ble, TKeys.tileBluetooth),
    ];

    final stepsValue = h.steps > 0 ? "${h.steps}" : "—";
    final heartRateValue = h.heartRate > 0 ? "${h.heartRate} bpm" : "— bpm";
    final spo2Value = h.spo2 > 0 ? "${h.spo2}%" : "—%";

    return ScreenScaffold(
      title: l10n.tr('home_title'),
      titleKey: TKeys.titleHome,
      subtitle: l10n.tr('home_subtitle'),
      child: Column(
        children: [
          SectionCard(
            title: l10n.tr('quick_access'),
            child: LayoutBuilder(
              builder: (context, c) {
                final cols = (c.maxWidth / 120).floor().clamp(3, 5);
                return GridView.builder(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  itemCount: items.length,
                  gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: cols,
                    mainAxisSpacing: 12,
                    crossAxisSpacing: 12,
                    childAspectRatio: 0.74,
                  ),
                  itemBuilder: (_, i) => _HomeTile(item: items[i]),
                );
              },
            ),
          ),
          const SizedBox(height: 8),
          SectionCard(
            title: l10n.tr('today_summary'),
            child: Row(
              children: [
                Expanded(
                  child: _SummaryTile(
                    key: TKeys.summarySteps,
                    icon: Icons.directions_walk,
                    label: l10n.tr('steps'),
                    value: stepsValue,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _SummaryTile(
                    key: TKeys.summaryHeart,
                    icon: Icons.favorite,
                    label: l10n.tr('heart_rate'),
                    value: heartRateValue,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _SummaryTile(
                    icon: Icons.health_and_safety,
                    label: l10n.tr('spo2'),
                    value: spo2Value,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _HomeItem {
  final IconData icon;
  final String label;
  final String route;
  final Key key;
  _HomeItem(this.icon, this.label, this.route, this.key);
}

class _HomeTile extends StatelessWidget {
  final _HomeItem item;
  const _HomeTile({required this.item});

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    return InkWell(
      key: item.key,
      onTap: () => Navigator.pushNamed(context, item.route),
      borderRadius: BorderRadius.circular(16),
      child: Container(
        decoration: BoxDecoration(
          color: cs.surface,
          borderRadius: BorderRadius.circular(16),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.15),
              blurRadius: 10,
              spreadRadius: 0,
              offset: const Offset(0, 4),
            ),
          ],
        ),
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              height: 44,
              width: 44,
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(12),
                color: cs.primary.withOpacity(0.08),
              ),
              alignment: Alignment.center,
              child: Icon(item.icon, size: 20),
            ),
            const SizedBox(height: 6),
            FittedBox(
              fit: BoxFit.scaleDown,
              child: Text(
                item.label,
                maxLines: 1,
                softWrap: false,
                overflow: TextOverflow.ellipsis,
                textAlign: TextAlign.center,
                style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 12, height: 1.1),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _SummaryTile extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  const _SummaryTile({super.key, required this.icon, required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    final th = Theme.of(context).textTheme;
    final cs = Theme.of(context).colorScheme;
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: cs.surface,
        borderRadius: BorderRadius.circular(14),
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 20),
          const SizedBox(height: 6),
          FittedBox(
            fit: BoxFit.scaleDown,
            child: Text(value, style: th.titleMedium?.copyWith(fontWeight: FontWeight.w800)),
          ),
          const SizedBox(height: 4),
          Text(
            label,
            maxLines: 1,
            softWrap: false,
            overflow: TextOverflow.ellipsis,
            style: th.bodySmall?.copyWith(color: cs.onSurface.withOpacity(0.8)),
          ),
        ],
      ),
    );
  }
}