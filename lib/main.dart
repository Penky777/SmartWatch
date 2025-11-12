import 'package:flutter/material.dart';
import 'theme/app_theme.dart';
import 'router.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const SmartWatchApp());
}

class SmartWatchApp extends StatelessWidget {
  const SmartWatchApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SmartWatchApp',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.dark(),
      onGenerateRoute: generateRoute,
      initialRoute: AppRoutes.home,
    );
  }
}
