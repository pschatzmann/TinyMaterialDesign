/**
 * @file table-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `TableLayout`.
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

// --- TableLayout (the control under test) -----------------------------------
// Not a widget - a grid with independently-sized columns/rows, unlike
// GridLayout's uniform cells. A wide first column next to a narrow second
// one, three rows tall.
constexpr int32_t kColumnWidths[] = {150, 60};
constexpr int32_t kRowHeights[] = {70, 70, 70};
TableLayout demoTable(Bounds(10, 20, kWidth - 20, kHeight - 40), kColumnWidths, 2, kRowHeights, 3);

Card<RGB565> wide0(Bounds(), "Wide", "Row 0");
Card<RGB565> narrow0(Bounds(), "N", nullptr);
Card<RGB565> wide1(Bounds(), "Wide", "Row 1");
Card<RGB565> narrow1(Bounds(), "N", nullptr);
Card<RGB565> wide2(Bounds(), "Wide", "Row 2");
Card<RGB565> narrow2(Bounds(), "N", nullptr);

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  wide0.bounds = demoTable.cellRect(0, 0);
  narrow0.bounds = demoTable.cellRect(0, 1);
  wide1.bounds = demoTable.cellRect(1, 0);
  narrow1.bounds = demoTable.cellRect(1, 1);
  wide2.bounds = demoTable.cellRect(2, 0);
  narrow2.bounds = demoTable.cellRect(2, 1);

  screen.addWidget(wide0);
  screen.addWidget(narrow0);
  screen.addWidget(wide1);
  screen.addWidget(narrow1);
  screen.addWidget(wide2);
  screen.addWidget(narrow2);

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
