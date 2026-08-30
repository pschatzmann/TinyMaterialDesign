/**
 * @file card.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Card`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Card (the control under test) ------------------------------------------
Card<RGB565> demoCard(Bounds(20, (app.height() - 140) / 2, app.width() - 40, 140), "TinyMaterialDesign",
                      "A simple elevated card with a title and word-wrapped body text.");

void setup() {
  Serial.begin(115200);
  app.begin();

  app.screen().addWidget(demoCard);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
