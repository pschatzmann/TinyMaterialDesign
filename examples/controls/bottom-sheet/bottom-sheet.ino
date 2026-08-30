/**
 * @file bottom-sheet.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `BottomSheet`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- BottomSheet (the control under test) ------------------------------------
BottomSheet<RGB565> demoSheet(Bounds(0, app.height() - 160, app.width(), 160), "Options");
ListItem<RGB565> sheetItem;

void setup() {
  app.begin();

  sheetItem = ListItem<RGB565>(demoSheet.itemRect(0), "Share");
  sheetItem.onClick = []() {
    printf("Bottom sheet item tapped\n");
    app.screen().dismissDialog();
  };
  demoSheet.addItem(sheetItem);
  demoSheet.onScrimTap = []() { app.screen().dismissDialog(); };

  // Shown immediately (no separate trigger control) - see Screen::presentDialog().
  app.screen().presentDialog(demoSheet);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
