/**
 * @file radio-button.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `RadioButton`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- RadioButton (the control under test) -----------------------------------
// A lone RadioButton never deselects itself on tap (radio semantics) -
// mutual exclusion across a group is RadioGroup's job, not shown here.
RadioButton<RGB565> demoRadio(Bounds((app.width() - 24) / 2, (app.height() - 24) / 2, 24, 24));

void setup() {
  Serial.begin(115200);
  app.begin();

  demoRadio.onSelected = []() { Serial.println("Radio selected"); };
  app.screen().addWidget(demoRadio);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
