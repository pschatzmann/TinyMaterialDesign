/**
 * @file carousel.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Carousel`.
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

// --- Carousel (the control under test) ---------------------------------------
// Items' own bounds are overwritten by addItem() (see Carousel::itemRect())
// - the Bounds() passed to each MediaCard here is just a placeholder.
Carousel<RGB565> demoCarousel(Bounds(0, (kHeight - 150) / 2, kWidth, 150), 120, 12);
MediaCard<RGB565> carouselItemA(Bounds(), "Jazz");
MediaCard<RGB565> carouselItemB(Bounds(), "Rock");
MediaCard<RGB565> carouselItemC(Bounds(), "Pop");

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  demoCarousel.addItem(carouselItemA);
  demoCarousel.addItem(carouselItemB);
  demoCarousel.addItem(carouselItemC);
  demoCarousel.onPageChange = [](int page) { printf("Carousel page: %d\n", page); };
  screen.addWidget(demoCarousel);

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
