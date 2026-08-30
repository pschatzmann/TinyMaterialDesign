/**
 * @file button.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Button`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Button (the control under test) --------------------------------------
Button<RGB565> demoButton(Bounds((app.width() - 120) / 2, (app.height() - 40) / 2, 120, 40), "Tap me");

void setup() {
  Serial.begin(115200);
  app.begin();

  demoButton.onClick = []() { Serial.println("Button tapped"); };
  app.screen().addWidget(demoButton);
}

void loop() {
  app.update();
}
