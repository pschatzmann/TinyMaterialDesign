/**
 * @file flow-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `FlowLayout`.
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

// --- FlowLayout (the control under test) ------------------------------------
// Not a widget - packs items of varying width left-to-right, wrapping to a
// new row once one would overflow. Each Chip reports its own width to next().
FlowLayout demoFlow(Bounds(10, 20, kWidth - 20, kHeight - 40));
Chip<RGB565> chipJazz(Bounds(), "Jazz");
Chip<RGB565> chipRock(Bounds(), "Rock");
Chip<RGB565> chipClassical(Bounds(), "Classical");
Chip<RGB565> chipPop(Bounds(), "Pop");
Chip<RGB565> chipHipHop(Bounds(), "Hip-Hop");
Chip<RGB565> chipElectronic(Bounds(), "Electronic");
Chip<RGB565>* chips[] = {&chipJazz, &chipRock, &chipClassical, &chipPop, &chipHipHop, &chipElectronic};
constexpr int32_t kChipWidths[] = {60, 60, 90, 60, 80, 100};

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  for (size_t i = 0; i < 6; ++i) {
    chips[i]->bounds = demoFlow.next(kChipWidths[i], /*height=*/36);
    screen.addWidget(*chips[i]);
  }
  printf("Flow content height: %d\n", demoFlow.totalHeight());

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
