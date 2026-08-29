/**
 * @file split-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `SplitLayout`.
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

// --- SplitLayout (the control under test) -----------------------------------
// Not a widget - divides a container into two panes along one axis. Here a
// 35/65 horizontal split, e.g. a narrow nav pane next to a content pane.
Card<RGB565> navCard(Bounds(), "Nav", "35%");
Card<RGB565> contentCard(Bounds(), "Content", "The remaining 65%, minus the gutter.");

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  SplitLayout demoSplit =
      SplitLayout::ratio(Bounds(0, 20, kWidth, kHeight - 40), LayoutAxis::Horizontal, 0.35f);
  navCard.bounds = demoSplit.firstRect();
  contentCard.bounds = demoSplit.secondRect();

  screen.addWidget(navCard);
  screen.addWidget(contentCard);

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
