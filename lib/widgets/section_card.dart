import 'package:flutter/material.dart';

class SectionCard extends StatelessWidget {
  final String title;
  final Widget? trailing;
  final Widget child;
  final EdgeInsetsGeometry? contentPadding;
  const SectionCard({
    super.key,
    required this.title,
    required this.child,
    this.trailing,
    this.contentPadding,
  });

  @override
  Widget build(BuildContext context) {
    final th = Theme.of(context);
    return Card(
      child: Padding(
        padding: contentPadding ?? const EdgeInsets.all(14),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Text(title, style: th.textTheme.titleMedium?.copyWith(fontWeight: FontWeight.w700)),
                const Spacer(),
                if (trailing != null) trailing!,
              ],
            ),
            const SizedBox(height: 10),
            child,
          ],
        ),
      ),
    );
  }
}
