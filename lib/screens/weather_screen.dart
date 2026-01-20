import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';

import '../Services/weather_service.dart';
import '../Services/location_service.dart';
import '../test_ids.dart';

class WeatherScreen extends StatefulWidget {
  const WeatherScreen({super.key});

  @override
  State<WeatherScreen> createState() => _WeatherScreenState();
}

class _WeatherScreenState extends State<WeatherScreen> {
  static const String openWeatherApiKey = '4eb6b61902804474c296d86e4d165b0b';
  final _weatherService = WeatherService(openWeatherApiKey);

  Weather? _weather;
  List<HourlyForecast> _hourly = [];
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

      // Načítaj oboje paralelne
      final results = await Future.wait([
        _weatherService.getCurrentWeather(pos.latitude, pos.longitude),
        _weatherService.getHourlyForecast(pos.latitude, pos.longitude),
      ]);

      if (!mounted) return;
      setState(() {
        _weather = results[0] as Weather;
        _hourly = results[1] as List<HourlyForecast>;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _error = e.toString();
      });
    } finally {
      if (!mounted) return;
      setState(() {
        _loading = false;
      });
    }
  }

  /// Ikona podľa OpenWeatherMap icon kódu
  IconData _weatherIcon(String iconCode) {
    switch (iconCode.substring(0, 2)) {
      case '01': return Icons.wb_sunny;
      case '02': return Icons.cloud_queue;
      case '03': return Icons.cloud;
      case '04': return Icons.cloud;
      case '09': return Icons.grain;
      case '10': return Icons.water_drop;
      case '11': return Icons.thunderstorm;
      case '13': return Icons.ac_unit;
      case '50': return Icons.foggy;
      default: return Icons.cloud_queue;
    }
  }

  Widget _hourBox(HourlyForecast forecast) {
    final hour = '${forecast.time.hour.toString().padLeft(2, '0')}:00';
    final temp = '${forecast.temp.round()}°';

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(hour, style: const TextStyle(fontWeight: FontWeight.w600)),
        const SizedBox(height: 6),
        Icon(_weatherIcon(forecast.icon), size: 20),
        const SizedBox(height: 6),
        Text(temp),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    final subtitle = (_weather?.cityName.isNotEmpty ?? false)
        ? '${_weather!.cityName}, dnes'
        : 'Načítavam...';

    return ScreenScaffold(
      title: "Počasie",
      titleKey: TKeys.titleWeather,
      subtitle: subtitle,
      actions: [
        IconButton(
          onPressed: _loadWeather,
          icon: const Icon(Icons.my_location),
        ),
      ],
      child: SingleChildScrollView(
        physics: const AlwaysScrollableScrollPhysics(
          parent: BouncingScrollPhysics(),
        ),
        child: Column(
          children: [
            SectionCard(
              title: "Aktuálne",
              child: _buildCurrentWeather(context),
            ),
            SectionCard(
              title: "Predpoveď",
              child: _buildHourlyForecast(),
            ),
            const SizedBox(height: 32),
          ],
        ),
      ),
    );
  }

  Widget _buildCurrentWeather(BuildContext context) {
    if (_loading && _weather == null) {
      return const Row(
        children: [
          SizedBox(
            width: 20,
            height: 20,
            child: CircularProgressIndicator(strokeWidth: 2),
          ),
          SizedBox(width: 12),
          Text("Načítavam počasie…"),
        ],
      );
    }

    if (_error != null && _weather == null) {
      return Text(
        "Chyba: $_error",
        key: TKeys.weatherLocationError,
        style: TextStyle(
          color: Theme.of(context).colorScheme.error,
        ),
      );
    }

    if (_weather == null) {
      return const Text("Počasie nie je k dispozícii");
    }

    return Column(
      children: [
        Row(
          children: [
            Icon(_weatherIcon(_weather!.icon), size: 34),
            const SizedBox(width: 12),
            Text(
              "${_weather!.description}, ${_weather!.temp.toStringAsFixed(1)}°C",
            ),
            const Spacer(),
            Text(
              "Pocitovo ${_weather!.feelsLike.toStringAsFixed(1)}°C",
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
        const SizedBox(height: 8),
        Row(
          children: [
            Icon(Icons.water_drop, size: 16, color: Colors.blue.shade300),
            const SizedBox(width: 4),
            Text("${_weather!.humidity}%"),
            const SizedBox(width: 16),
            Icon(Icons.air, size: 16, color: Colors.grey.shade400),
            const SizedBox(width: 4),
            Text("${_weather!.windSpeed.toStringAsFixed(1)} m/s"),
          ],
        ),
      ],
    );
  }

  Widget _buildHourlyForecast() {
    if (_loading && _hourly.isEmpty) {
      return const Center(
        child: Padding(
          padding: EdgeInsets.all(16),
          child: CircularProgressIndicator(strokeWidth: 2),
        ),
      );
    }

    if (_hourly.isEmpty) {
      return const Text("Predpoveď nie je k dispozícii");
    }

    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        children: _hourly
            .map((f) => Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12),
          child: _hourBox(f),
        ))
            .toList(),
      ),
    );
  }
}