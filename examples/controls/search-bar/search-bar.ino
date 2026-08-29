/**
 * @file search-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `SearchBar`.
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

// --- SearchBar (the control under test) ---------------------------------------
// Tap to focus (a blinking cursor appears); pair with a Keyboard to
// actually type into it - omitted here to keep this a single-control demo.
SearchBar<RGB565> demoSearch(Bounds(20, (kHeight - 48) / 2, kWidth - 40, 48), "Search");

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  demoSearch.onSubmit = []() { printf("Search submitted: %s\n", demoSearch.text().c_str()); };
  screen.addWidget(demoSearch);

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
