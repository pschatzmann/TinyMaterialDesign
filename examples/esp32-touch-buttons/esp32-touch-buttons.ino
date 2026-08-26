/**
 * @file esp32-touch-buttons.ino
 * @brief Reference sketch for a real touch panel, using one of TinyGPU's
 * one-call LCDBoard setups (display + touch controller together) instead of
 * wiring the SPI/I2C bus and driver by hand. Shows a Button, Switch and
 * Slider.
 *
 * NOTE: this is a documentation reference only - it is not part of the
 * CMake build (there is no ESP32/touch-hardware emulation path to run it
 * against here).
 *
 * Pick exactly one board below (see TinyGPU/Boards/LCDBoardsESP32.h for the
 * full pinout/wiring notes of each): uncomment the one matching your
 * hardware and comment out (or remove) the rest.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoardsESP32.h>
#include <TinyGPU/Drivers/DeviceOutput.h>

// "2.8" ESP32-S3 Display" (FBBA0125-002 / ESP32-S3 Hosyond Display) - ILI9341
// 240x320 SPI + FT6336G capacitive touch.
// LCDBoardESP32S3_2_8Display board;

// Guition ESP32-S3 4.3" 480x272 Capacitive Touch Display (JC4827W543C_I) -
// NV3041A QSPI + GT911 capacitive touch.
// LCDBoardGuitionESP32S3_4_3Display board;

// ESP32 Arduino LVGL WiFi&Bluetooth 2.4" LCD (ESP32-2432S028R / ESP32 Cheap
// Yellow Display) - ILI9341 240x320 SPI + CST816S capacitive touch. Same
// board pinout TinyGPU's own examples (e.g. examples/color-test) target.
LCDBoardGuitionESP32_LVGL_2_4Display board;

// Guition ESP32-P4 4.3" 480x800 Capacitive Touch Display (JC4880P443C_I_W) -
// ST7701 MIPI-DSI + GT911 capacitive touch. ESP32-P4 only.
// LCDBoardGuitionESP32H4_4_3Display board;

Surface<RGB565> surface(board.width(), board.height(), FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
Screen<RGB565> screen;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();

Button<RGB565> okButton(Bounds(70, 240, 100, 40), "OK");
Switch<RGB565> wifiSwitch(Bounds(70, 180, 48, 28));
Slider<RGB565> brightnessSlider(Bounds(20, 120, 200, 24), 0.0f, 100.0f, 75.0f);

void setup() {
  Serial.begin(115200);

  board.begin();
  display.begin();
  surface.begin();

  okButton.onClick = []() { Serial.println("OK tapped"); };
  wifiSwitch.onChange = [](bool value) { Serial.println(value ? "WiFi on" : "WiFi off"); };
  brightnessSlider.onChange = [](float value) { Serial.printf("Brightness: %.0f\n", value); };

  screen.addWidget(okButton);
  screen.addWidget(wifiSwitch);
  screen.addWidget(brightnessSlider);

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) { return screen.isDraggableAt(x, y); };
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());
  screen.draw(surface, theme);
  display.writeData(surface);
}
