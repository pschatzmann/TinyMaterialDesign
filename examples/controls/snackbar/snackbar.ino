/**
 * @file snackbar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Snackbar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Snackbar (the control under test) ---------------------------------------
Snackbar<RGB565> demoSnackbar(Bounds(16, app.height() - 64, app.width() - 32, 48));

void setup() {
  Serial.begin(115200);
  app.begin();

  demoSnackbar.onAction = []() { Serial.println("Snackbar action tapped"); };
  app.screen().addFixedWidget(demoSnackbar);
  demoSnackbar.show("Saved", "Undo");
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
