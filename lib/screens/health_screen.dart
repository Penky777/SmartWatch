import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class HealthScreen extends StatelessWidget {
  const HealthScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 1,
    child: Center(child: Text('Zdravie – placeholder')),
  );
}
