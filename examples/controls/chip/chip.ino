/**
 * @file chip.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Chip`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Chip (the control under test) -------------------------------------------
Chip<RGB565> demoChip(Bounds((app.width() - 100) / 2, (app.height() - 32) / 2, 100, 32), "Filter",
                      /*selectable=*/true);

void setup() {
  Serial.begin(115200);
  app.begin();

  demoChip.onChange = [](bool selected) { Serial.print("Chip: "); Serial.println(selected ? "selected" : "unselected"); };
  app.screen().addWidget(demoChip);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
