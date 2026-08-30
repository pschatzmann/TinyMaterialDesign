/**
 * @file segmented-button.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `SegmentedButton`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- SegmentedButton (the control under test) -------------------------------
SegmentedButton<RGB565> demoSegmented(Bounds(20, (app.height() - 36) / 2, app.width() - 40, 36));

void setup() {
  app.begin();

  demoSegmented.addSegment("Day");
  demoSegmented.addSegment("Week");
  demoSegmented.addSegment("Month");
  demoSegmented.onChange = [](uint32_t mask) { printf("Segmented selection mask: %u\n", mask); };
  app.screen().addWidget(demoSegmented);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
