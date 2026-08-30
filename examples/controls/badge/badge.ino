/**
 * @file badge.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Badge`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Badge (the control under test) -----------------------------------------
// Non-interactive - typically overlaid on a corner of another widget (e.g.
// an IconButton's bounds); shown centered on its own here.
Badge<RGB565> demoBadge(Bounds((app.width() - 24) / 2, (app.height() - 24) / 2, 24, 24), "5");

void setup() {
  app.begin();

  app.screen().addWidget(demoBadge);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
