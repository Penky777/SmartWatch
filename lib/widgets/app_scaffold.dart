// lib/widgets/app_scaffold.dart

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
    final content = body ?? child ?? const SizedBox.shrink();

    // ✅ Jednoduché pozadie - sivé pre light, čierne pre dark
    final backgroundColor = Theme.of(context).scaffoldBackgroundColor;

    return Scaffold(
      backgroundColor: backgroundColor,
      appBar: appBar ??
          AppBar(
            backgroundColor: backgroundColor,
            elevation: 0,
            automaticallyImplyLeading: Navigator.canPop(context),
            title: Text(title ?? '', key: titleKey),
            centerTitle: false,
            actions: actions,
          ),
      body: SafeArea(child: content),
      bottomNavigationBar: bottom,
    );
  }
}