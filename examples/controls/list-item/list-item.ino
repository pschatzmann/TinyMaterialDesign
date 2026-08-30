/**
 * @file list-item.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `ListItem`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- ListItem (the control under test) ---------------------------------------
ListItem<RGB565> demoListItem(Bounds(20, (app.height() - 48) / 2, app.width() - 40, 48), "Settings");

void setup() {
  app.begin();

  demoListItem.onClick = []() { printf("List item tapped\n"); };
  app.screen().addWidget(demoListItem);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
