import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class WeatherScreen extends StatelessWidget {
  const WeatherScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 4,
    child: Center(child: Text('Počasie – placeholder')),
  );
}
