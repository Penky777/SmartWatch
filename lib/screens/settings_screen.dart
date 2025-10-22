import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 7,
    child: Center(child: Text('Nastavenia – placeholder')),
  );
}
