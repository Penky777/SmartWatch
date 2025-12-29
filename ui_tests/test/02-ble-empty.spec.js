const { byValueKey } = require("appium-flutter-finder");

describe("BLE - empty state", () => {
  it("shows empty text when no devices found", async () => {
    await driver.elementClick(byValueKey("tile_bluetooth"));
    await driver.execute("flutter:waitFor", byValueKey("title_ble"), 10000);

    // stop scan
    await driver.execute("flutter:waitForTappable", byValueKey("ble_btn_stop"), 10000);
    await driver.elementClick(byValueKey("ble_btn_stop"));

    await driver.execute("flutter:waitFor", byValueKey("ble_empty_text"), 10000);
  });
});
