const { byValueKey } = require("appium-flutter-finder");

async function denyPermissionIfDialogAppears() {
  await driver.switchContext("NATIVE_APP");

  const denySelectors = [
    "id=com.android.permissioncontroller:id/permission_deny_button",
    "id=com.android.permissioncontroller:id/permission_deny_and_dont_ask_again_button",
    "id=com.android.packageinstaller:id/permission_deny_button"
  ];

  for (const sel of denySelectors) {
    const els = await driver.$$(sel);
    if (els.length) {
      await els[0].click();
      break;
    }
  }

  await driver.switchContext("FLUTTER");
}

describe("Weather - location denied", () => {
  it("shows error when location permission denied", async () => {
    await driver.elementClick(byValueKey("tile_weather"));
    await driver.execute("flutter:waitFor", byValueKey("title_weather"), 10000);

    await denyPermissionIfDialogAppears();

    await driver.execute("flutter:waitFor", byValueKey("weather_location_error"), 10000);
  });
});
