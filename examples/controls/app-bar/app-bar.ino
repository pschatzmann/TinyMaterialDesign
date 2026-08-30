/**
 * @file app-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `AppBar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- AppBar (the control under test) -----------------------------------------
// leading/trailing are plain pointer fields (see AppBar::leading/trailing) -
// the IconButtons themselves are declared here and positioned/attached in
// setup(), the same pattern kitchen-sink.ino uses.
AppBar<RGB565> demoAppBar(Bounds(0, 0, app.width(), 48), "App Bar");
IconButton<RGB565> appBarMenu;
IconButton<RGB565> appBarAdd;

void setup() {
  app.begin();

  appBarMenu = IconButton<RGB565>(demoAppBar.leadingRect(), drawMenu<RGB565>);
  demoAppBar.leading = &appBarMenu;
  appBarMenu.onClick = []() { printf("Menu tapped\n"); };

  appBarAdd = IconButton<RGB565>(demoAppBar.trailingRect(), drawPlus<RGB565>);
  demoAppBar.trailing = &appBarAdd;
  appBarAdd.onClick = []() { printf("Add tapped\n"); };

  // Demonstrates setColorOverride(): recolors just this bar to the theme's
  // primary color, independent of the rest of the app.screen().
  demoAppBar.setColorOverride(app.theme().colors.primary, app.theme().colors.onPrimary);

  app.screen().addFixedWidget(demoAppBar);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
