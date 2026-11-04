/**
 * @file wifi-login.ino
 * @brief WiFi selection and login with WifiLoginPage: the page is shown on
 * startup when the device is not connected; after the login (or Cancel) a
 * second page shows the state of the connection, with a
 * button to come back. The button next to the SSID field opens the list of
 * the networks.
 *
 * On an ESP32 the real WiFi is used (Esp32Wifi: the credentials are stored
 * in the flash); on the desktop a simulated one (SimulatedWifi: the password
 * of the secured networks is "demo").
 */
#include <TinyMaterialDesign.h>

#if defined(ESP32)
#include <SPI.h>   // display (TinyGPU)
#include <Wire.h>  // touch (TinyGPU)
#include "TinyMaterialDesign/Wifi/WifiEsp32.h"
Esp32Wifi wifi;
#else
SimulatedWifi wifi;
#endif

App<RGB565> app(DefaultBoard, defaultDarkTheme<RGB565>());
Keyboard<RGB565> keyboard;
WifiLoginPage<RGB565> login(keyboard, app.screen());

// the "application": the state of the connection
Container<RGB565> home;
Label<RGB565> state;
Button<RGB565> change;

void showLogin() {
  home.visible = false;
  login.visible = true;
  login.onShow();
  app.screen().invalidate();
}

void showHome() {
  login.onHide();
  login.visible = false;
  home.visible = true;
  app.screen().invalidate();
}

void setup() {
  Serial.begin(115200);
  app.begin();
#if defined(ESP32)
  wifi.begin();  // connects with the stored credentials
#endif
  const int32_t w = static_cast<int32_t>(app.width());
  const int32_t h = static_cast<int32_t>(app.height());
  keyboard.bounds = Bounds(0, h - 170, w, 170);
  keyboard.visible = false;  // shown when a text field is tapped

  home.bounds = Bounds(0, 0, w, h);
  state = Label<RGB565>(Bounds(16, 40, w - 32, 20), "", TypographyRole::kBody, TextAlign::kCenter);
  state.setOnBackground(true);
  change = Button<RGB565>(Bounds(16, 80, w - 32, 36), "WiFi settings", ButtonVariant::kOutlined);
  change.onClick = []() { showLogin(); };
  app.screen().addWidget(home);
  home.addChild(state);
  home.addChild(change);

  login.setWifi(&wifi);
  login.bounds = Bounds(0, 0, w, h);
  login.onDone = []() { showHome(); };
  app.screen().addWidget(login);
  login.build();
  app.screen().addFixedWidget(keyboard);  // last: on top

  // the login page on startup unless already connected (simulation: never)
  if (wifi.state() == WifiControl::State::kConnected) {
    showHome();
  } else {
    showLogin();
  }
}

void loop() {
  const uint32_t now = millis();
  if (login.visible && login.tick(now)) app.screen().invalidate();
  static uint32_t lastState = 0;
  if (home.visible && now - lastState > 500) {
    lastState = now;
    char buf[64];
    if (wifi.state() == WifiControl::State::kConnected) {
      snprintf(buf, sizeof(buf), "%s (%s)", wifi.ssid().c_str(), wifi.ip().c_str());
    } else {
      snprintf(buf, sizeof(buf), "No WiFi");
    }
    if (state.text() != buf) {
      state.setText(buf);
      app.screen().invalidate();
    }
  }
  app.update();
}
