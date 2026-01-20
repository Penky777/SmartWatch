//
//  Generated file. Do not edit.
//

import FlutterMacOS
import Foundation

import a_bridge
import device_info_plus
import file_selector_macos
import geolocator_apple
import package_info_plus
import reactive_ble_mobile
import shared_preferences_foundation

func RegisterGeneratedPlugins(registry: FlutterPluginRegistry) {
  ABridgePlugin.register(with: registry.registrar(forPlugin: "ABridgePlugin"))
  DeviceInfoPlusMacosPlugin.register(with: registry.registrar(forPlugin: "DeviceInfoPlusMacosPlugin"))
  FileSelectorPlugin.register(with: registry.registrar(forPlugin: "FileSelectorPlugin"))
  GeolocatorPlugin.register(with: registry.registrar(forPlugin: "GeolocatorPlugin"))
  FPPPackageInfoPlusPlugin.register(with: registry.registrar(forPlugin: "FPPPackageInfoPlusPlugin"))
  ReactiveBlePlugin.register(with: registry.registrar(forPlugin: "ReactiveBlePlugin"))
  SharedPreferencesPlugin.register(with: registry.registrar(forPlugin: "SharedPreferencesPlugin"))
}
