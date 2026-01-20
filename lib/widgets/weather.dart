import 'package:flutter/material.dart';
import '../services/weather_service.dart';
import '../services/location_service.dart';

class WeatherWidget extends StatefulWidget {
  const WeatherWidget({super.key});

  @override
  State<WeatherWidget> createState() => _WeatherWidgetState();
}

class _WeatherWidgetState extends State<WeatherWidget> {
  final _service = WeatherService('4eb6b61902804474c296d86e4d165b0b');
  Weather? _weather;
  bool _loading = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    _loadWeather();
  }

  Future<void> _loadWeather() async {
    setState(() {
      _loading = true;
      _error = null;
    });

    try {
      final pos = await getCurrentPosition();
      final w = await _service.getCurrentWeather(pos.latitude, pos.longitude);
      if (!mounted) return;
      setState(() => _weather = w);
    } catch (e) {
      if (!mounted) return;
      setState(() => _error = e.toString());
    } finally {
      if (!mounted) return;
      setState(() => _loading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_loading) return const Text('Načítavam počasie…');
    if (_error != null) return Text('Chyba: $_error');
    if (_weather == null) return const Text('Počasie nie je k dispozícii');

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          '${_weather!.temp.toStringAsFixed(1)} °C',
          style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
        ),
        Text(_weather!.description),
      ],
    );
  }
}
