import 'package:flutter/material.dart';

class AppScaffold extends StatelessWidget {
  /// Ak chceš vlastný AppBar, môžeš ho sem poslať.
  final PreferredSizeWidget? appBar;

  /// Titulok pre defaultný AppBar (použije sa len ak `appBar == null`).
  final String? title;

  /// Akcie vpravo hore v defaultnom AppBare.
  final List<Widget>? actions;

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
    this.title,
    this.actions,
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
        appBar: appBar ??
            AppBar(
              backgroundColor: Colors.transparent,
              elevation: 0,
              automaticallyImplyLeading: Navigator.canPop(context),
              title: Text(title ?? ''),
              centerTitle: false,
              actions: actions,
            ),
        body: SafeArea(child: content),
        bottomNavigationBar: bottom,
      ),
    );
  }
}
