import 'package:flutter_driver/driver_extension.dart';
import 'package:my_flutter/main.dart' as app;

void main() {
  // MUSÍ byť pred spustením appky
  enableFlutterDriverExtension();

  // spustí tvoju normálnu appku
  app.main();
}
