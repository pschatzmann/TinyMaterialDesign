/**
 * @file text-area.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `TextArea`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- TextArea (the control under test) ---------------------------------------
// Tap to focus (a blinking cursor appears); pair with a Keyboard (see the
// keyboard example) to actually type into it.
TextArea<RGB565> demoArea(Bounds(20, (app.height() - 140) / 2, app.width() - 40, 140), "Notes",
                          "Write something...");

void setup() {
  Serial.begin(115200);
  app.begin();

  app.screen().addWidget(demoArea);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
