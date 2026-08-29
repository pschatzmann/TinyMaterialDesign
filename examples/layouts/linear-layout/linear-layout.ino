/**
 * @file linear-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `LinearLayout`.
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

// --- LinearLayout (the control under test) ----------------------------------
// Not a widget - splits a container into slices along one axis. Button B
// gets twice the space of A and C via a weights array.
LinearLayout demoRow(Bounds(10, (kHeight - 48) / 2, kWidth - 20, 48), LayoutAxis::Horizontal);
Button<RGB565> buttonA(Bounds(), "A");
Button<RGB565> buttonB(Bounds(), "B (2x)");
Button<RGB565> buttonC(Bounds(), "C");

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  float weights[] = {1.0f, 2.0f, 1.0f};
  buttonA.bounds = demoRow.itemRect(0, weights, 3);
  buttonB.bounds = demoRow.itemRect(1, weights, 3);
  buttonC.bounds = demoRow.itemRect(2, weights, 3);

  buttonA.onClick = []() { printf("Button A tapped\n"); };
  buttonB.onClick = []() { printf("Button B tapped\n"); };
  buttonC.onClick = []() { printf("Button C tapped\n"); };
  screen.addWidget(buttonA);
  screen.addWidget(buttonB);
  screen.addWidget(buttonC);

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
