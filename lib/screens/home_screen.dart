import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../router.dart';
import '../test_ids.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final items = [
      _HomeItem(Icons.directions_run, "Aktivita", AppRoutes.activity, TKeys.tileActivity),
      _HomeItem(Icons.favorite, "Zdravie", AppRoutes.health, TKeys.tileHealth),
      _HomeItem(Icons.cloud, "Počasie", AppRoutes.weather, TKeys.tileWeather),
      _HomeItem(Icons.calendar_month, "Kalendár", AppRoutes.calendar, TKeys.tileCalendar),
      _HomeItem(Icons.notifications, "Notifikácie", AppRoutes.notifications, TKeys.tileNotifications),
      _HomeItem(Icons.watch, "Watchfaces", AppRoutes.watchfaces, TKeys.tileWatchfaces),
      _HomeItem(Icons.settings, "Nastavenia", AppRoutes.settings, TKeys.tileSettings),
      _HomeItem(Icons.bluetooth, "Bluetooth", AppRoutes.ble, TKeys.tileBluetooth),
    ];


    return ScreenScaffold(
      title: "SmartWatch",
      titleKey: TKeys.titleHome,
      subtitle: "Všetko dôležité na jednom mieste",
      child: Column(
        children: [
          SectionCard(
            title: "Rýchly prístup",
            child: LayoutBuilder(
              builder: (context, c) {
                // šírka dlaždice ~120px → vyjde pekný počet stĺpcov
                final cols = (c.maxWidth / 120).floor().clamp(3, 5);
                return GridView.builder(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  itemCount: items.length,
                  gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: cols,
                    mainAxisSpacing: 12,
                    crossAxisSpacing: 12,
                    childAspectRatio: 0.74, // trošku vyššie dlaždice
                  ),
                  itemBuilder: (_, i) => _HomeTile(item: items[i]),
                );
              },
            ),
          ),
          const SizedBox(height: 8),
          SectionCard(
            title: "Dnešné zhrnutie",
            child: Row(
              children: const [
                Expanded(child: _SummaryTile(icon: Icons.directions_walk, label: "Kroky", value: "8 240")),
                SizedBox(width: 12),
                Expanded(child: _SummaryTile(icon: Icons.favorite, label: "Tep", value: "72 bpm")),
                SizedBox(width: 12),
                Expanded(child: _SummaryTile(icon: Icons.nightlight, label: "Spánok", value: "7h 25m")),
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
          // border: Border.all(...),  // odstránené
          boxShadow: [
            // jemný „glow“ namiesto rámčeka (voliteľné)
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
                // border: Border.all(...), // odstránené
                color: cs.primary.withOpacity(0.08), // jemné pozadie ikony (voliteľné)
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
  const _SummaryTile({required this.icon, required this.label, required this.value});

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
