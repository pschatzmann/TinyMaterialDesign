/**
 * @file banner.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Banner`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Banner (the control under test) -----------------------------------------
// Pinned to the bottom edge (see addFixedWidget() below) rather than just
// under an app bar, so it reads as a persistent bottom-of-screen notice.
Banner<RGB565> demoBanner(Bounds(0, app.height() - 80, app.width(), 80), "You're offline. Check your connection.");

void setup() {
  app.begin();

  app.screen().addFixedWidget(demoBanner);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
