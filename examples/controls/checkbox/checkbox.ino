/**
 * @file checkbox.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Checkbox`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Checkbox (the control under test) --------------------------------------
Checkbox<RGB565> demoCheckbox(Bounds((app.width() - 24) / 2, (app.height() - 24) / 2, 24, 24), true);

void setup() {
  Serial.begin(115200);
  app.begin();

  demoCheckbox.onChange = [](bool checked) { Serial.print("Checkbox: "); Serial.println(checked ? "checked" : "unchecked"); };
  app.screen().addWidget(demoCheckbox);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
