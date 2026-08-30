/**
 * @file carousel.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Carousel`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Carousel (the control under test) ---------------------------------------
// Items' own bounds are overwritten by addItem() (see Carousel::itemRect())
// - the Bounds() passed to each MediaCard here is just a placeholder.
Carousel<RGB565> demoCarousel(Bounds(0, (app.height() - 150) / 2, app.width(), 150), 120, 12);
MediaCard<RGB565> carouselItemA(Bounds(), "Jazz");
MediaCard<RGB565> carouselItemB(Bounds(), "Rock");
MediaCard<RGB565> carouselItemC(Bounds(), "Pop");

void setup() {
  app.begin();

  demoCarousel.addItem(carouselItemA);
  demoCarousel.addItem(carouselItemB);
  demoCarousel.addItem(carouselItemC);
  demoCarousel.onPageChange = [](int page) { printf("Carousel page: %d\n", page); };
  app.screen().addWidget(demoCarousel);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
