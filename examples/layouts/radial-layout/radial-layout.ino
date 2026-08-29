/**
 * @file radial-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `RadialLayout`.
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

// --- RadialLayout (the control under test) ----------------------------------
// Not a widget - places item rects evenly spaced around a circle, angle 0 at
// 12 o'clock and clockwise, like a dial/menu on a round display.
RadialLayout demoDial(Bounds(0, 40, kWidth, kWidth), /*radius=*/90);
Button<RGB565> item0(Bounds(), "12");
Button<RGB565> item1(Bounds(), "2");
Button<RGB565> item2(Bounds(), "4");
Button<RGB565> item3(Bounds(), "6");
Button<RGB565> item4(Bounds(), "8");
Button<RGB565> item5(Bounds(), "10");
Button<RGB565>* items[] = {&item0, &item1, &item2, &item3, &item4, &item5};

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  for (size_t i = 0; i < 6; ++i) {
    items[i]->bounds = demoDial.itemRect(static_cast<int>(i), 6, /*width=*/40, /*height=*/40);
    screen.addWidget(*items[i]);
  }

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
