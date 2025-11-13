import 'package:flutter/material.dart';
import '../widgets/screen_scafold.dart';
import '../widgets/section_card.dart';

import '../Services/weather_service.dart';
import '../Services/location_service.dart';

class WeatherScreen extends StatefulWidget {
  const WeatherScreen({super.key});


  @override
  State<WeatherScreen> createState() => _WeatherScreenState();
}

class _WeatherScreenState extends State<WeatherScreen> {
  static const String openWeatherApiKey = '4eb6b61902804474c296d86e4d165b0b';
  // sem daj svoj API key z OpenWeatherMap
  final _weatherService = WeatherService(openWeatherApiKey);

  Weather? _weather;
  bool _loading = false;
  String? _error;

  // malý názov metódy (konvencia)
  Widget _hourBox({required String h, required String t}) {
    return Column(
      children: [
        Text(h, style: const TextStyle(fontWeight: FontWeight.w600)),
        const SizedBox(height: 6),
        const Icon(Icons.cloud_queue, size: 20),
        const SizedBox(height: 6),
        Text(t),
      ],
    );
  }

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
      // zistí polohu telefónu
      final pos = await getCurrentPosition();

      // zavolá API
      final w = await _weatherService.getCurrentWeather(
        pos.latitude,
        pos.longitude,
      );

      if (!mounted) return;
      setState(() {
        _weather = w;
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

  @override
  Widget build(BuildContext context) {
    // TU bol problém – cityName? neexistuje
    final subtitle = (_weather?.cityName.isNotEmpty ?? false)
        ? '${_weather!.cityName}, dnes'
        : 'Trebišov, dnes';

    return ScreenScaffold(
      title: "Počasie",
      subtitle: subtitle,
      actions: [
        IconButton(
          onPressed: _loadWeather, // refresh po kliknutí
          icon: const Icon(Icons.my_location),
        ),
      ],
      child: ListView(
        children: [
          SectionCard(
            title: "Aktuálne",
            child: _buildCurrentWeather(context),
          ),
          SectionCard(
            title: "Ďalšie hodiny",
            // forecast je zatiaľ fake
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                _hourBox(h: "12:00", t: "9°"),
                _hourBox(h: "15:00", t: "10°"),
                _hourBox(h: "18:00", t: "7°"),
                _hourBox(h: "21:00", t: "5°"),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCurrentWeather(BuildContext context) {
    if (_loading) {
      return Row(
        children: [
          const SizedBox(
            width: 20,
            height: 20,
            child: CircularProgressIndicator(strokeWidth: 2),
          ),
          const SizedBox(width: 12),
          const Text("Načítavam počasie…"),
        ],
      );
    }

    if (_error != null) {
      return Text(
        "Chyba: $_error",
        style: TextStyle(
          color: Theme.of(context).colorScheme.error,
        ),
      );
    }

    if (_weather == null) {
      return const Text("Počasie nie je k dispozícii");
    }

    return Row(
      children: [
        const Icon(Icons.cloud, size: 34),
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
    );
  }
}
