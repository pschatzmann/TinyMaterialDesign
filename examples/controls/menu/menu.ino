/**
 * @file menu.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Menu`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Menu (the control under test) -------------------------------------------
Menu<RGB565> demoMenu(Bounds((app.width() - 160) / 2, (app.height() - 40) / 2, 160, 40));
ListItem<RGB565> menuItem;

void setup() {
  Serial.begin(115200);
  app.begin();

  menuItem = ListItem<RGB565>(demoMenu.itemRect(0), "Option 1");
  menuItem.onClick = []() {
    Serial.println("Menu item tapped");
    app.screen().dismissDialog();
  };
  demoMenu.addItem(menuItem);
  demoMenu.onOutsideTap = []() { app.screen().dismissDialog(); };

  // Shown immediately (no separate trigger control) - see Screen::presentDialog().
  app.screen().presentDialog(demoMenu);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
