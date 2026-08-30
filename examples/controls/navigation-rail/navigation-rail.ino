/**
 * @file navigation-rail.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `NavigationRail`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- NavigationRail (the control under test) ---------------------------------
NavigationRail<RGB565> demoNavRail(Bounds(0, 0, 72, app.height()));

void setup() {
  app.begin();

  demoNavRail.addDestination(drawPlus<RGB565>, "Add");
  demoNavRail.addDestination(drawMinus<RGB565>, "Remove");
  demoNavRail.addDestination(drawMenu<RGB565>, "More");
  demoNavRail.onChange = [](int index) { printf("Nav destination: %d\n", index); };
  app.screen().addFixedWidget(demoNavRail);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
