// lib/screens/watchfaces_screen.dart

import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../test_ids.dart';
import '../l10n/app_localizations.dart';

class WatchfacesScreen extends StatelessWidget {
  const WatchfacesScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = context.l10n;
    final faces = List.generate(8, (i) => "Watchface ${i + 1}");

    return ScreenScaffold(
      titleKey: TKeys.titleWatchfaces,
      title: l10n.tr('watchfaces_title'),
      subtitle: l10n.tr('watchfaces_subtitle'),
      child: ListView(
        children: [
          SectionCard(
            title: l10n.tr('collection'),
            child: GridView.builder(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              itemCount: faces.length,
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: 3,
                crossAxisSpacing: 10,
                mainAxisSpacing: 10,
              ),
              itemBuilder: (_, i) => _FaceTile(name: faces[i]),
            ),
          ),
        ],
      ),
    );
  }
}

class _FaceTile extends StatelessWidget {
  final String name;
  const _FaceTile({required this.name});

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    return InkWell(
      onTap: () {},
      borderRadius: BorderRadius.circular(16),
      child: Container(
        decoration: BoxDecoration(
          color: cs.surface,
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: cs.primary.withOpacity(0.15)),
        ),
        padding: const EdgeInsets.all(10),
        child: Column(
          children: [
            const Expanded(child: Icon(Icons.watch, size: 40)),
            const SizedBox(height: 6),
            Text(name, textAlign: TextAlign.center, maxLines: 1, overflow: TextOverflow.ellipsis),
          ],
        ),
      ),
    );
  }
}