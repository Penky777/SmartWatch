import 'package:flutter/material.dart';
import '../widgets/app_scaffold.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});
  @override
  Widget build(BuildContext context) {
    final text = Theme.of(context).textTheme;
    return AppScaffold(
      index: 0,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [
            Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text('12:34', style: text.displaySmall),
              const SizedBox(height: 2),
              Text('Streda • 24. 9.', style: text.labelMedium?.copyWith(color: Colors.white70)),
            ]),
            Row(children: const [
              Icon(Icons.bluetooth_connected, size: 18, color: Colors.white70),
              SizedBox(width: 8), Text('78%', style: TextStyle(color: Colors.white70)),
            ]),
          ]),
          const SizedBox(height: 12),
          Expanded(
            child: GridView.count(
              crossAxisCount: 2,
              crossAxisSpacing: 8, mainAxisSpacing: 8, childAspectRatio: 1.4,
              children: const [
                _StatCard(title:'Kroky', value:'4 230'),
                _StatCard(title:'Kalórie', value:'560 kcal'),
                _StatCard(title:'Tep', value:'72 bpm', sub:'Priemer 7d: 68'),
                _StatCard(title:'SpO₂', value:'98%'),
              ],
            ),
          ),
          Row(children: [
            Expanded(child: FilledButton(onPressed: (){
              ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('Hľadám hodinky…')));
            }, child: const Text('Pripojiť'))),
            const SizedBox(width: 8),
            Expanded(child: OutlinedButton(onPressed: (){}, child: const Text('Start stopky'))),
            const SizedBox(width: 8),
            Expanded(child: OutlinedButton(onPressed: (){}, child: const Text('AI otázka'))),
          ]),
        ],
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  final String title; final String value; final String? sub;
  const _StatCard({required this.title, required this.value, this.sub});
  @override
  Widget build(BuildContext context) {
    final t = Theme.of(context).textTheme;
    return Card(
      clipBehavior: Clip.antiAlias,
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
          Text(title, style: t.labelLarge?.copyWith(color: Colors.white70)),
          const Spacer(),
          Text(value, style: t.headlineSmall),
          if (sub != null) ...[
            const SizedBox(height: 4),
            Text(sub!, style: t.bodySmall?.copyWith(color: Colors.white70)),
          ],
        ]),
      ),
    );
  }
}
