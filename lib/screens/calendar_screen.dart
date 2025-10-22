import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class CalendarScreen extends StatelessWidget {
  const CalendarScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 3,
    child: Center(child: Text('Kalendár – placeholder')),
  );
}
