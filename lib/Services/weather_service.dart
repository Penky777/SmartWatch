import 'dart:convert';
import 'package:http/http.dart' as http;

class Weather {
  final String cityName;
  final double temp;
  final double feelsLike;
  final String description;
  final String icon;
  final int humidity;
  final double windSpeed;

  Weather({
    required this.cityName,
    required this.temp,
    required this.feelsLike,
    required this.description,
    required this.icon,
    this.humidity = 0,
    this.windSpeed = 0,
  });

  factory Weather.fromJson(Map<String, dynamic> json) {
    return Weather(
      cityName: json['name'] ?? '',
      temp: (json['main']['temp'] as num).toDouble(),
      feelsLike: (json['main']['feels_like'] as num).toDouble(),
      description: json['weather'][0]['description'] ?? '',
      icon: json['weather'][0]['icon'] ?? '01d',
      humidity: json['main']['humidity'] ?? 0,
      windSpeed: (json['wind']?['speed'] as num?)?.toDouble() ?? 0,
    );
  }
}

/// Hodinová predpoveď
class HourlyForecast {
  final DateTime time;
  final double temp;
  final String icon;
  final String description;

  HourlyForecast({
    required this.time,
    required this.temp,
    required this.icon,
    required this.description,
  });

  factory HourlyForecast.fromJson(Map<String, dynamic> json) {
    return HourlyForecast(
      time: DateTime.fromMillisecondsSinceEpoch(json['dt'] * 1000),
      temp: (json['main']['temp'] as num).toDouble(),
      icon: json['weather'][0]['icon'] ?? '01d',
      description: json['weather'][0]['description'] ?? '',
    );
  }
}

class WeatherService {
  final String apiKey;
  static const _baseUrl = 'https://api.openweathermap.org/data/2.5';

  WeatherService(this.apiKey);

  /// Aktuálne počasie
  Future<Weather> getCurrentWeather(double lat, double lon) async {
    final url = Uri.parse(
      '$_baseUrl/weather?lat=$lat&lon=$lon&appid=$apiKey&units=metric&lang=sk',
    );

    final response = await http.get(url);

    if (response.statusCode == 200) {
      return Weather.fromJson(jsonDecode(response.body));
    } else {
      throw Exception('Nepodarilo sa načítať počasie: ${response.statusCode}');
    }
  }

  /// Hodinová predpoveď (najbližších 8 položiek = cca 24 hodín v 3h intervaloch)
  Future<List<HourlyForecast>> getHourlyForecast(double lat, double lon) async {
    final url = Uri.parse(
      '$_baseUrl/forecast?lat=$lat&lon=$lon&appid=$apiKey&units=metric&lang=sk&cnt=8',
    );

    final response = await http.get(url);

    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      final list = data['list'] as List;
      return list.map((item) => HourlyForecast.fromJson(item)).toList();
    } else {
      throw Exception('Nepodarilo sa načítať predpoveď: ${response.statusCode}');
    }
  }
}