import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

class AppScaffold extends StatelessWidget {
  final Widget child;
  final int index;
  const AppScaffold({super.key, required this.child, required this.index});

  @override
  Widget build(BuildContext context) {
    final items = const [
      (Icons.home,        '/',            'Domov'),
      (Icons.favorite,    '/health',      'Zdravie'),
      (Icons.timer,       '/activity',    'Aktivity'),
      (Icons.calendar_month,'/calendar',  'Kalendár'),
      (Icons.cloud,       '/weather',     'Počasie'),
      (Icons.notifications,'/notifications','Notif'),
      (Icons.watch,       '/watchfaces',  'Ciferníky'),
      (Icons.settings,    '/settings',    'Nast.'),
    ];

    return Scaffold(
      body: SafeArea(child: Padding(padding: const EdgeInsets.all(12), child: child)),
      bottomNavigationBar: NavigationBar(
        selectedIndex: index,
        onDestinationSelected: (i) => context.go(items[i].$2),
        destinations: [
          for (final it in items) NavigationDestination(icon: Icon(it.$1), label: it.$3),
        ],
      ),
    );
  }
}
