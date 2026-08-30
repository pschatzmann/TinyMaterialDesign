/**
 * @file slider.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Slider`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Slider (the control under test) ----------------------------------------
Slider<RGB565> demoSlider(Bounds(20, (app.height() - 24) / 2, app.width() - 40, 24), 0.0f, 100.0f, 40.0f);

void setup() {
  Serial.begin(115200);
  app.begin();

  demoSlider.onChange = [](float value) { Serial.print("Slider: "); Serial.println(static_cast<int>(value)); };
  app.screen().addWidget(demoSlider);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
