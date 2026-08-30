/**
 * @file drawer.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Drawer`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Drawer (the control under test) -----------------------------------------
Drawer<RGB565> demoDrawer(Bounds(0, 0, 220, app.height()));
ListItem<RGB565> drawerItem;

void setup() {
  app.begin();

  drawerItem = ListItem<RGB565>(demoDrawer.itemRect(0), "Home");
  drawerItem.setSelected(true);
  drawerItem.onClick = []() {
    printf("Drawer item tapped\n");
    app.screen().dismissDialog();
  };
  demoDrawer.addItem(drawerItem);
  demoDrawer.onScrimTap = []() { app.screen().dismissDialog(); };

  // Shown immediately (no separate trigger control) - see Screen::presentDialog().
  app.screen().presentDialog(demoDrawer);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
