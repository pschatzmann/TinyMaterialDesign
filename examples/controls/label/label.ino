/**
 * @file label.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Label`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Label (the control under test) -----------------------------------------
Label<RGB565> demoLabel(Bounds(20, (app.height() - 24) / 2, app.width() - 40, 24), "Hello, Material Design!",
                        TypographyRole::kTitle, TextAlign::kCenter);

void setup() {
  app.begin();

  app.screen().addWidget(demoLabel);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
