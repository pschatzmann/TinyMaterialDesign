/**
 * @file divider.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Divider`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Divider (the control under test) ---------------------------------------
Divider<RGB565> demoDivider(Bounds(20, (app.height() - 2) / 2, app.width() - 40, 2));

void setup() {
  Serial.begin(115200);
  app.begin();

  app.screen().addWidget(demoDivider);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
