/**
 * @file media-card.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `MediaCard`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- MediaCard (the control under test) ---------------------------------------
MediaCard<RGB565> demoMediaCard(Bounds((app.width() - 140) / 2, (app.height() - 140) / 2, 140, 140), "Jazz");

void setup() {
  Serial.begin(115200);
  app.begin();

  demoMediaCard.onClick = []() { Serial.println("Media card tapped"); };
  app.screen().addWidget(demoMediaCard);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
