import 'package:flutter_driver/driver_extension.dart';
import 'main.dart' as app;

void main() {
  enableFlutterDriverExtension(); // MUSÍ byť prvé
  app.main();                     // potom spustí tvoju appku
}
