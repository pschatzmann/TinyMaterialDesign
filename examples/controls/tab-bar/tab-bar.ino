/**
 * @file tab-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `TabBar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- TabBar (the control under test) -----------------------------------------
TabBar<RGB565> demoTabs(Bounds(0, (app.height() - 40) / 2, app.width(), 40));

void setup() {
  app.begin();

  demoTabs.addTab("One");
  demoTabs.addTab("Two");
  demoTabs.addTab("Three");
  demoTabs.onChange = [](int index) { printf("Tab selected: %d\n", index); };
  app.screen().addWidget(demoTabs);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
