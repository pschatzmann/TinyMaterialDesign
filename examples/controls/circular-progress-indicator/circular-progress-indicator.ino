/**
 * @file circular-progress-indicator.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `CircularProgressIndicator`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- CircularProgressIndicator (the control under test) ---------------------
CircularProgressIndicator<RGB565> demoProgress(Bounds((app.width() - 48) / 2, (app.height() - 48) / 2, 48, 48),
                                               0.0f, /*indeterminate=*/true);

void setup() {
  app.begin();

  app.screen().addWidget(demoProgress);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
