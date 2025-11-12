import 'package:flutter/material.dart';

class AppScaffold extends StatelessWidget {
  final PreferredSizeWidget? appBar;

  /// Novšie použitie
  final Widget? body;

  /// Staršie použitie (alias na `body`)
  final Widget? child;

  final Widget? bottom;

  /// Ak používaš index na spodnú navigáciu, nechávam ho tu (inak pokojne zmaž).
  final int? index;

  const AppScaffold({
    super.key,
    this.appBar,
    this.body,
    this.child,
    this.bottom,
    this.index,
  });

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    final content = body ?? child ?? const SizedBox.shrink();

    return Container(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [cs.primary.withOpacity(0.12), Colors.transparent],
          begin: Alignment.topCenter,
          end: Alignment.center,
        ),
      ),
      child: Scaffold(
        backgroundColor: Colors.transparent,
        appBar: appBar,
        body: SafeArea(child: content),
        bottomNavigationBar: bottom,
      ),
    );
  }
}
