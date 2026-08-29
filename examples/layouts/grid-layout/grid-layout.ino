/**
 * @file grid-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `GridLayout`.
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

// --- GridLayout (the control under test) ------------------------------------
// Not a widget - a small calculator that wraps a row of equal-size cells to
// fit its container's width. As many columns as fit are used automatically;
// here that's 2 columns of 100x70 cells with the default 8px spacing.
GridLayout demoGrid(Bounds(10, 20, kWidth - 20, kHeight - 40), /*cellWidth=*/100, /*cellHeight=*/70);
Button<RGB565> cellA(Bounds(), "1");
Button<RGB565> cellB(Bounds(), "2");
Button<RGB565> cellC(Bounds(), "3");
Button<RGB565> cellD(Bounds(), "4");
Button<RGB565>* cells[] = {&cellA, &cellB, &cellC, &cellD};

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  for (size_t i = 0; i < 4; ++i) {
    cells[i]->bounds = demoGrid.cellRect(static_cast<int>(i));
    cells[i]->onClick = [i]() { printf("Grid cell %d tapped\n", static_cast<int>(i)); };
    screen.addWidget(*cells[i]);
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
