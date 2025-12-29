const { byValueKey } = require("appium-flutter-finder");

async function openTile(tileKey, titleKey) {
  await driver.execute("flutter:waitForTappable", byValueKey(tileKey));
  await driver.elementClick(byValueKey(tileKey));

  await driver.execute("flutter:waitFor", byValueKey(titleKey), 10000);

  await driver.back();
  await driver.execute("flutter:waitFor", byValueKey("title_home"), 10000);
}

describe("Home - Quick access navigation", () => {
  it("opens all screens and asserts titles", async () => {
    // Home screen assertion
    await driver.execute("flutter:waitFor", byValueKey("title_home"), 10000);

    // (voliteľné) summary keys, ak si ich pridal
    // await driver.execute("flutter:waitFor", byValueKey("summary_steps"), 10000);

    await openTile("tile_activity", "title_activity");
    await openTile("tile_health", "title_health");
    await openTile("tile_weather", "title_weather");
    await openTile("tile_calendar", "title_calendar");
    await openTile("tile_notifications", "title_notifications");
    await openTile("tile_watchfaces", "title_watchfaces");
    await openTile("tile_settings", "title_settings");
    await openTile("tile_bluetooth", "title_ble");
  });
});
