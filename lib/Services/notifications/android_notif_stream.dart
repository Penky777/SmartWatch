import 'package:flutter/services.dart';

class AndroidNotifStream {
  static const _events = EventChannel('watch/notif_events');
  static const _methods = MethodChannel('watch/notif_methods');

  Stream<Map<String, dynamic>> get stream =>
      _events.receiveBroadcastStream().map((e) => Map<String, dynamic>.from(e));

  Future<void> openAccessSettings() async {
    await _methods.invokeMethod('openNotificationAccess');
  }

  Future isAccessEnabled() async {
    final res = await _methods.invokeMethod('isNotificationAccessEnabled');
    return (res == true);
  }
}
