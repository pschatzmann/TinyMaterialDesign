/**
 * @file text-field.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `TextField`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- TextField (the control under test) ---------------------------------------
// Tap to focus (a blinking cursor appears); pair with a Keyboard (see the
// keyboard example) to actually type into it.
TextField<RGB565> demoField(Bounds(20, (app.height() - 48) / 2, app.width() - 40, 48), "Name", "Your name");

void setup() {
  Serial.begin(115200);
  app.begin();

  demoField.onSubmit = []() { Serial.print("Submitted: "); Serial.println(demoField.text().c_str()); };
  app.screen().addWidget(demoField);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
