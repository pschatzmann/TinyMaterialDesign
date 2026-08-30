/**
 * @file linear-progress-indicator.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `LinearProgressIndicator`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- LinearProgressIndicator (the control under test) -----------------------
LinearProgressIndicator<RGB565> demoProgress(Bounds(20, (app.height() - 8) / 2, app.width() - 40, 8), 0.0f,
                                             /*indeterminate=*/true);

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
