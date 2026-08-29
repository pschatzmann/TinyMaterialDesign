/**
 * @file menu.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Menu`.
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

// --- Menu (the control under test) -------------------------------------------
Menu<RGB565> demoMenu(Bounds((kWidth - 160) / 2, (kHeight - 40) / 2, 160, 40));
ListItem<RGB565> menuItem;

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  menuItem = ListItem<RGB565>(demoMenu.itemRect(0), "Option 1");
  menuItem.onClick = []() {
    printf("Menu item tapped\n");
    screen.dismissDialog();
  };
  demoMenu.addItem(menuItem);
  demoMenu.onOutsideTap = []() { screen.dismissDialog(); };

  // Shown immediately (no separate trigger control) - see Screen::presentDialog().
  screen.presentDialog(demoMenu);

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
