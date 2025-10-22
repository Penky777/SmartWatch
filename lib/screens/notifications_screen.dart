import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class NotificationsScreen extends StatelessWidget {
  const NotificationsScreen({super.key});
  @override
  Widget build(BuildContext context) => const AppScaffold(
    index: 5,
    child: Center(child: Text('Notifikácie – placeholder')),
  );
}
