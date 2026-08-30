/**
 * @file tooltip.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Tooltip`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Tooltip (the control under test) -----------------------------------------
Tooltip<RGB565> demoTooltip;

void setup() {
  app.begin();

  // showFor() measures text using the theme, so the widget must already be
  // registered (and so themed) with Screen before calling it.
  app.screen().addFixedWidget(demoTooltip);
  demoTooltip.showFor(Bounds(app.width() / 2 - 10, app.height() / 2 - 10, 20, 20), "Hint text",
                      /*durationMs=*/60000);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
