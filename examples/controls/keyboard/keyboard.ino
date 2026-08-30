/**
 * @file keyboard.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Keyboard`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Keyboard (the control under test) ---------------------------------------
// Normally shown/targeted via a TextField's/TextArea's onFocus (see
// Keyboard::manage()); forced visible here directly so this stays a
// single-control demo - keys still respond, they just have no field to
// insert characters into.
Keyboard<RGB565> demoKeyboard(Bounds(0, app.height() - 190, app.width(), 190));

void setup() {
  app.begin();

  demoKeyboard.visible = true;
  app.screen().addFixedWidget(demoKeyboard);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
