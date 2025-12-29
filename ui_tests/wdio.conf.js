exports.config = {
  runner: "local",
  framework: "mocha",
  reporters: ["spec"],
  specs: ["./test/**/*.spec.js"],
  mochaOpts: { timeout: 180000 },

  // ✅ úplne vypni paralelizáciu
  maxInstances: 1,

  hostname: "127.0.0.1",
  port: 4723,
  path: "/",

  connectionRetryTimeout: 300000,
  connectionRetryCount: 1,

  capabilities: [
    {
      // ✅ aj tu natvrdo 1
      maxInstances: 1,

      platformName: "Android",
      "appium:automationName": "FlutterIntegration",
      "appium:deviceName": "Android Emulator",

      // dôležité – APK
      "appium:app": process.env.APP,

      // stabilita
      "appium:newCommandTimeout": 300,
      "appium:adbExecTimeout": 300000,
      "appium:uiautomator2ServerInstallTimeout": 180000,
      "appium:androidInstallTimeout": 300000,

      // logicky
      "appium:autoGrantPermissions": true
    }
  ]
};
