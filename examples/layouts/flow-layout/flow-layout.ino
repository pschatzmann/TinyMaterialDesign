/**
 * @file flow-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `FlowLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- FlowLayout (the control under test) ------------------------------------
// Not a widget - packs items of varying width left-to-right, wrapping to a
// new row once one would overflow. Each Chip reports its own width to next().
FlowLayout demoFlow(Bounds(10, 20, app.width() - 20, app.height() - 40));
Chip<RGB565> chipJazz(Bounds(), "Jazz");
Chip<RGB565> chipRock(Bounds(), "Rock");
Chip<RGB565> chipClassical(Bounds(), "Classical");
Chip<RGB565> chipPop(Bounds(), "Pop");
Chip<RGB565> chipHipHop(Bounds(), "Hip-Hop");
Chip<RGB565> chipElectronic(Bounds(), "Electronic");
Chip<RGB565>* chips[] = {&chipJazz, &chipRock, &chipClassical, &chipPop, &chipHipHop, &chipElectronic};
constexpr int32_t kChipWidths[] = {60, 60, 90, 60, 80, 100};

void setup() {
  Serial.begin(115200);
  app.begin();

  for (size_t i = 0; i < 6; ++i) {
    chips[i]->bounds = demoFlow.next(kChipWidths[i], /*height=*/36);
    app.screen().addWidget(*chips[i]);
  }
  Serial.print("Flow content height: "); Serial.println(demoFlow.totalHeight());
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
