/**
 * @file search-bar.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `SearchBar`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- SearchBar (the control under test) ---------------------------------------
// Tap to focus (a blinking cursor appears); pair with a Keyboard to
// actually type into it - omitted here to keep this a single-control demo.
SearchBar<RGB565> demoSearch(Bounds(20, (app.height() - 48) / 2, app.width() - 40, 48), "Search");

void setup() {
  app.begin();

  demoSearch.onSubmit = []() { printf("Search submitted: %s\n", demoSearch.text().c_str()); };
  app.screen().addWidget(demoSearch);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
