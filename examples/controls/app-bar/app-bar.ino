/**
 * @file app-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `AppBar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoards.h>
#include <cstdio>

constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

#ifdef ESP32
LCDBoardGuitionESP32_LVGL_2_4Display board;
#else
LCDBoardDesktopSDL board(kWidth, kHeight);
#endif
Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
Screen<RGB565> screen(theme);

// --- AppBar (the control under test) -----------------------------------------
// leading/trailing are plain pointer fields (see AppBar::leading/trailing) -
// the IconButtons themselves are declared here and positioned/attached in
// setup(), the same pattern kitchen-sink.ino uses.
AppBar<RGB565> demoAppBar(Bounds(0, 0, kWidth, 48), "App Bar");
IconButton<RGB565> appBarMenu;
IconButton<RGB565> appBarAdd;

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  appBarMenu = IconButton<RGB565>(demoAppBar.leadingRect(), drawMenu<RGB565>);
  demoAppBar.leading = &appBarMenu;
  appBarMenu.onClick = []() { printf("Menu tapped\n"); };

  appBarAdd = IconButton<RGB565>(demoAppBar.trailingRect(), drawPlus<RGB565>);
  demoAppBar.trailing = &appBarAdd;
  appBarAdd.onClick = []() { printf("Add tapped\n"); };

  // Demonstrates setColorOverride(): recolors just this bar to the theme's
  // primary color, independent of the rest of the screen.
  demoAppBar.setColorOverride(theme.colors.primary, theme.colors.onPrimary);

  screen.addFixedWidget(demoAppBar);

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) { return screen.isDraggableAt(x, y); };
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());
  // Screen tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  if (screen.isDirty()) {
    screen.draw(surface);
    display.writeData(surface);
  }
}
