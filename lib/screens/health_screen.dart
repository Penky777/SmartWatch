import 'dart:async';

import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';

import '../di.dart';
import '../ble/ble_repository.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';
import '../widgets/metric_chip.dart';
import '../test_ids.dart';

class HealthScreen extends StatefulWidget {
  const HealthScreen({super.key});

  @override
  State<HealthScreen> createState() => _HealthScreenState();
}

class _HealthScreenState extends State<HealthScreen> {
  BleRepository? _repo;
  StreamSubscription<HealthData?>? _healthSub;
  StreamSubscription<BleStatus>? _statusSub;

  @override
  void initState() {
    super.initState();

    try {
      if (getIt.isRegistered<BleRepository>()) {
        _repo = getIt<BleRepository>();

        _healthSub = _repo!.healthDataStream.listen((_) {
          if (mounted) setState(() {});
        });

        _statusSub = _repo!.status.listen((_) {
          if (mounted) setState(() {});
        });
      }
    } catch (e) {
      debugPrint('HealthScreen: BleRepository not available: $e');
    }
  }

  @override
  void dispose() {
    _healthSub?.cancel();
    _statusSub?.cancel();
    super.dispose();
  }

  String _stressLevel(int heartRate) {
    if (heartRate == 0) return '—';
    if (heartRate < 60) return 'nízky';
    if (heartRate < 80) return 'normálny';
    if (heartRate < 100) return 'mierne zvýšený';
    return 'vysoký';
  }

  List<FlSpot> _toSpots(List<int> data) {
    return data
        .asMap()
        .entries
        .map((e) => FlSpot(e.key.toDouble(), e.value.toDouble()))
        .toList();
  }

  double _calcTrendProgress(HealthData h) {
    if (h.minHr == 0 || h.maxHr == 0 || h.maxHr == h.minHr) return 0.5;
    return ((h.heartRate - h.minHr) / (h.maxHr - h.minHr)).clamp(0.0, 1.0);
  }

  @override
  Widget build(BuildContext context) {
    final isConnected = _repo?.currentStatus == BleStatus.connected;
    final colorScheme = Theme.of(context).colorScheme;
    final h = _repo?.healthData ?? HealthData();

    return ScreenScaffold(
      title: "Zdravie",
      titleKey: TKeys.titleHealth,
      actions: [
        IconButton(
          onPressed: () {},
          icon: const Icon(Icons.refresh),
        ),
      ],
      // OPRAVA: SingleChildScrollView namiesto ListView - spoľahlivejší scroll
      child: SingleChildScrollView(
        physics: const AlwaysScrollableScrollPhysics(
          parent: BouncingScrollPhysics(),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Connection status banner
            if (!isConnected)
              Container(
                margin: const EdgeInsets.only(bottom: 12),
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.watch_off, color: Colors.orange),
                    SizedBox(width: 8),
                    Text('Hodinky nie sú pripojené'),
                  ],
                ),
              ),

            // Realtime metriky
            SectionCard(
              title: "Realtime",
              child: Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  MetricChip(
                    icon: Icons.favorite,
                    label: "Tep",
                    value: h.heartRate > 0 ? "${h.heartRate} bpm" : "— bpm",
                  ),
                  MetricChip(
                    icon: Icons.health_and_safety,
                    label: "SpO₂",
                    value: h.spo2 > 0 ? "${h.spo2}%" : "—%",
                  ),
                  MetricChip(
                    icon: Icons.directions_walk,
                    label: "Kroky",
                    value: "${h.steps}",
                  ),
                  MetricChip(
                    icon: Icons.self_improvement,
                    label: "Stres",
                    value: _stressLevel(h.heartRate),
                  ),
                ],
              ),
            ),

            // Graf tepu
            SectionCard(
              title: "Tep - história",
              child: SizedBox(
                height: 180,
                child: h.heartRateHistory.isEmpty
                    ? const Center(
                  child: Text(
                    'Čakám na dáta z hodiniek...',
                    style: TextStyle(color: Colors.grey),
                  ),
                )
                    : LineChart(
                  LineChartData(
                    gridData: FlGridData(
                      show: true,
                      drawVerticalLine: false,
                      horizontalInterval: 20,
                      getDrawingHorizontalLine: (value) => FlLine(
                        color: colorScheme.outline.withOpacity(0.2),
                        strokeWidth: 1,
                      ),
                    ),
                    titlesData: FlTitlesData(
                      leftTitles: AxisTitles(
                        sideTitles: SideTitles(
                          showTitles: true,
                          reservedSize: 40,
                          interval: 20,
                          getTitlesWidget: (value, meta) => Text(
                            value.toInt().toString(),
                            style: TextStyle(
                              fontSize: 10,
                              color: colorScheme.onSurfaceVariant,
                            ),
                          ),
                        ),
                      ),
                      bottomTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      topTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      rightTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                    ),
                    borderData: FlBorderData(show: false),
                    minY: (h.minHr - 10).toDouble().clamp(40, 200),
                    maxY: (h.maxHr + 10).toDouble().clamp(60, 220),
                    lineBarsData: [
                      LineChartBarData(
                        spots: _toSpots(h.heartRateHistory),
                        isCurved: true,
                        curveSmoothness: 0.3,
                        color: Colors.redAccent,
                        barWidth: 3,
                        isStrokeCapRound: true,
                        dotData: FlDotData(
                          show: true,
                          getDotPainter: (spot, percent, bar, index) {
                            final isLast =
                                index == h.heartRateHistory.length - 1;
                            return FlDotCirclePainter(
                              radius: isLast ? 5 : 2,
                              color: Colors.redAccent,
                              strokeWidth: isLast ? 2 : 0,
                              strokeColor: Colors.white,
                            );
                          },
                        ),
                        belowBarData: BarAreaData(
                          show: true,
                          color: Colors.redAccent.withOpacity(0.15),
                        ),
                      ),
                    ],
                    lineTouchData: LineTouchData(
                      touchTooltipData: LineTouchTooltipData(
                        getTooltipItems: (spots) => spots
                            .map((spot) => LineTooltipItem(
                          '${spot.y.toInt()} bpm',
                          const TextStyle(
                            color: Colors.white,
                            fontWeight: FontWeight.bold,
                          ),
                        ))
                            .toList(),
                      ),
                    ),
                  ),
                ),
              ),
            ),

            // Graf SpO2
            SectionCard(
              title: "SpO₂ - história",
              child: SizedBox(
                height: 150,
                child: h.spo2History.isEmpty
                    ? const Center(
                  child: Text(
                    'Čakám na dáta z hodiniek...',
                    style: TextStyle(color: Colors.grey),
                  ),
                )
                    : LineChart(
                  LineChartData(
                    gridData: FlGridData(
                      show: true,
                      drawVerticalLine: false,
                      horizontalInterval: 2,
                      getDrawingHorizontalLine: (value) => FlLine(
                        color: colorScheme.outline.withOpacity(0.2),
                        strokeWidth: 1,
                      ),
                    ),
                    titlesData: FlTitlesData(
                      leftTitles: AxisTitles(
                        sideTitles: SideTitles(
                          showTitles: true,
                          reservedSize: 40,
                          interval: 2,
                          getTitlesWidget: (value, meta) => Text(
                            '${value.toInt()}%',
                            style: TextStyle(
                              fontSize: 10,
                              color: colorScheme.onSurfaceVariant,
                            ),
                          ),
                        ),
                      ),
                      bottomTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      topTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      rightTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                    ),
                    borderData: FlBorderData(show: false),
                    minY: 90,
                    maxY: 100,
                    lineBarsData: [
                      LineChartBarData(
                        spots: _toSpots(h.spo2History),
                        isCurved: true,
                        curveSmoothness: 0.3,
                        color: Colors.blueAccent,
                        barWidth: 3,
                        isStrokeCapRound: true,
                        dotData: FlDotData(
                          show: true,
                          getDotPainter: (spot, percent, bar, index) {
                            final isLast =
                                index == h.spo2History.length - 1;
                            return FlDotCirclePainter(
                              radius: isLast ? 5 : 2,
                              color: Colors.blueAccent,
                              strokeWidth: isLast ? 2 : 0,
                              strokeColor: Colors.white,
                            );
                          },
                        ),
                        belowBarData: BarAreaData(
                          show: true,
                          color: Colors.blueAccent.withOpacity(0.15),
                        ),
                      ),
                    ],
                    lineTouchData: LineTouchData(
                      touchTooltipData: LineTouchTooltipData(
                        getTooltipItems: (spots) => spots
                            .map((spot) => LineTooltipItem(
                          '${spot.y.toInt()}%',
                          const TextStyle(
                            color: Colors.white,
                            fontWeight: FontWeight.bold,
                          ),
                        ))
                            .toList(),
                      ),
                    ),
                  ),
                ),
              ),
            ),

            // Trend bar
            SectionCard(
              title: "Trend (session)",
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    "🡻 min: ${h.minHr > 0 ? '${h.minHr} bpm' : '—'}   "
                        "🡹 max: ${h.maxHr > 0 ? '${h.maxHr} bpm' : '—'}",
                  ),
                  const SizedBox(height: 8),
                  LinearProgressIndicator(value: _calcTrendProgress(h)),
                  const SizedBox(height: 4),
                  Text(
                    'Meraní: ${h.heartRateHistory.length}',
                    style: TextStyle(
                      fontSize: 12,
                      color: colorScheme.onSurfaceVariant,
                    ),
                  ),
                ],
              ),
            ),

            // Extra padding na spodku pre lepší scroll
            const SizedBox(height: 32),
          ],
        ),
      ),
    );
  }
}