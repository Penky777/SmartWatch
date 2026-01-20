import 'dart:convert';
import 'package:http/http.dart' as http;

class Weather {
  final double temp;
  final double feelsLike;
  final String description;
  final String icon;
  final String cityName;

  const Weather({
    required this.temp,
    required this.feelsLike,
    required this.description,
    required this.icon,
    required this.cityName,
  });

  factory Weather.fromJson(Map<String, dynamic> json) {
    return Weather(
      temp: (json['main']['temp'] as num).toDouble(),
      feelsLike: (json['main']['feels_like'] as num).toDouble(),
      description: (json['weather'][0]['description'] as String),
      icon: (json['weather'][0]['icon'] as String),
      cityName: (json['name'] as String? ?? ''),
    );
  }
}

class WeatherService {
  final String apiKey;

  WeatherService(this.apiKey);

  Future<Weather> getCurrentWeather(double lat, double lon) async {
    final uri = Uri.https('api.openweathermap.org', '/data/2.5/weather', {
      'lat': lat.toString(),
      'lon': lon.toString(),
      'units': 'metric',
      'lang': 'sk',
      'appid': apiKey,
    });

    final response = await http.get(uri);
    print(response.statusCode);
    print(response.body);

    if (response.statusCode != 200) {
      throw Exception('Failed to load weather: ${response.statusCode}');
    }

    final data = jsonDecode(response.body) as Map<String, dynamic>;
    return Weather.fromJson(data);

  }
}
