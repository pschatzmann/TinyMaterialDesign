/**
 * @file fab.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `FloatingActionButton (FAB)`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- FloatingActionButton / FAB (the control under test) -------------------
FAB demoFab(Bounds((app.width() - 140) / 2, (app.height() - 56) / 2, 140, 56), drawPlus<RGB565>, "Add");

void setup() {
  app.begin();

  demoFab.onClick = []() { printf("FAB tapped\n"); };
  app.screen().addWidget(demoFab);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
