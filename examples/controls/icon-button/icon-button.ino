/**
 * @file icon-button.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `IconButton`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- IconButton (the control under test) -----------------------------------
IconButton<RGB565> demoIconButton(Bounds((app.width() - 56) / 2, (app.height() - 56) / 2, 56, 56),
                                  drawPlus<RGB565>, IconButtonVariant::kFilled);

void setup() {
  app.begin();

  demoIconButton.onClick = []() { printf("Icon button tapped\n"); };
  app.screen().addWidget(demoIconButton);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
