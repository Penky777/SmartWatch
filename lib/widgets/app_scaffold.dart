import 'package:flutter/material.dart';

class AppScaffold extends StatelessWidget {
  final PreferredSizeWidget? appBar;
  final String? title;

  final Key? titleKey;

  final List<Widget>? actions;
  final Widget? body;
  final Widget? child;
  final Widget? bottom;
  final int? index;

  const AppScaffold({
    super.key,
    this.appBar,
    this.title,
    this.titleKey,
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

              // ✅ UPRAV (len key pridáš)
              title: Text(title ?? '', key: titleKey),

              centerTitle: false,
              actions: actions,
            ),
        body: SafeArea(child: content),
        bottomNavigationBar: bottom,
      ),
    );
  }
}

