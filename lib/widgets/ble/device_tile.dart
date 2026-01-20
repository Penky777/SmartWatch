import 'package:flutter/material.dart';

class DeviceTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final int rssi;
  final VoidCallback onTap;
  const DeviceTile({
    super.key,
    required this.title,
    required this.subtitle,
    required this.rssi,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    final rssiText = rssi == 0 ? "" : "• RSSI $rssi dBm";
    return Card(
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        title: Text(
          title.isNotEmpty ? title : "(bez mena)",
          style: const TextStyle(fontWeight: FontWeight.w700),
        ),
        subtitle: Text("$subtitle  $rssiText"),
        trailing: Icon(Icons.chevron_right, color: cs.primary),
        onTap: onTap,
      ),
    );
  }
}
