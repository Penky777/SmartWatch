import 'package:flutter/material.dart';

class TKeys {
  // HOME tiles (Quick access)
  static const tileActivity = ValueKey('tile_activity');
  static const tileHealth = ValueKey('tile_health');
  static const tileWeather = ValueKey('tile_weather');
  static const tileCalendar = ValueKey('tile_calendar');
  static const tileNotifications = ValueKey('tile_notifications');
  static const tileWatchfaces = ValueKey('tile_watchfaces');
  static const tileSettings = ValueKey('tile_settings');
  static const tileBluetooth = ValueKey('tile_bluetooth');

  // HOME summary tiles
  static const summarySteps = ValueKey('summary_steps');
  static const summaryHeart = ValueKey('summary_heart');
  static const summarySleep = ValueKey('summary_sleep');

  // AppScaffold titles (assertions)
  static const titleHome = ValueKey('title_home');
  static const titleActivity = ValueKey('title_activity');
  static const titleHealth = ValueKey('title_health');
  static const titleWeather = ValueKey('title_weather');
  static const titleCalendar = ValueKey('title_calendar');
  static const titleNotifications = ValueKey('title_notifications');
  static const titleWatchfaces = ValueKey('title_watchfaces');
  static const titleSettings = ValueKey('title_settings');
  static const titleBle = ValueKey('title_ble');

  // BLE screen
  static const bleBtnSearch = ValueKey('ble_btn_search');
  static const bleBtnStop = ValueKey('ble_btn_stop');
  static const bleEmptyText = ValueKey('ble_empty_text');

  // Weather negative
  static const weatherLocationError = ValueKey('weather_location_error');
}
