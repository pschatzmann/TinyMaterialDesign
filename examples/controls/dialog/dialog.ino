/**
 * @file dialog.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Dialog`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Dialog (the control under test) -----------------------------------------
Dialog<RGB565> demoDialog(Bounds(20, (app.height() - 160) / 2, app.width() - 40, 160), "Hello",
                          "This is a simple modal dialog with one action.");
Button<RGB565> dialogOk(Bounds(0, 0, 80, 36), "OK");

void setup() {
  app.begin();

  dialogOk.bounds = demoDialog.actionRect(0, 1);
  dialogOk.onClick = []() {
    printf("Dialog OK tapped\n");
    app.screen().dismissDialog();
  };
  demoDialog.addAction(dialogOk);

  // Shown immediately (no separate trigger control) - see Screen::presentDialog().
  app.screen().presentDialog(demoDialog);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
