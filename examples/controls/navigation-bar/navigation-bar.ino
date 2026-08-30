/**
 * @file navigation-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `NavigationBar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- NavigationBar (the control under test) ----------------------------------
NavigationBar<RGB565> demoNavBar(Bounds(0, app.height() - 64, app.width(), 64));

void setup() {
  Serial.begin(115200);
  app.begin();

  demoNavBar.addDestination(drawPlus<RGB565>, "Add");
  demoNavBar.addDestination(drawMinus<RGB565>, "Remove");
  demoNavBar.addDestination(drawMenu<RGB565>, "More");
  demoNavBar.onChange = [](int index) { Serial.print("Nav destination: "); Serial.println(index); };
  app.screen().addFixedWidget(demoNavBar);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
