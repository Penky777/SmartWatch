import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class ActivityScreen extends StatelessWidget {
  const ActivityScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 2,
    child: Center(child: Text('Aktivity – placeholder')),
  );
}
