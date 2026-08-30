/**
 * @file switch.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Switch`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Switch (the control under test) ----------------------------------------
Switch<RGB565> demoSwitch(Bounds((app.width() - 48) / 2, (app.height() - 28) / 2, 48, 28));

void setup() {
  Serial.begin(115200);
  app.begin();

  demoSwitch.onChange = [](bool value) { Serial.print("Switch: "); Serial.println(value ? "on" : "off"); };
  app.screen().addWidget(demoSwitch);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
