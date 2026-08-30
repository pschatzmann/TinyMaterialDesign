/**
 * @file linear-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `LinearLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- LinearLayout (the control under test) ----------------------------------
// Not a widget - splits a container into slices along one axis. Button B
// gets twice the space of A and C via a weights array.
LinearLayout demoRow(Bounds(10, (app.height() - 48) / 2, app.width() - 20, 48), LayoutAxis::Horizontal);
Button<RGB565> buttonA(Bounds(), "A");
Button<RGB565> buttonB(Bounds(), "B (2x)");
Button<RGB565> buttonC(Bounds(), "C");

void setup() {
  Serial.begin(115200);
  app.begin();

  float weights[] = {1.0f, 2.0f, 1.0f};
  buttonA.bounds = demoRow.itemRect(0, weights, 3);
  buttonB.bounds = demoRow.itemRect(1, weights, 3);
  buttonC.bounds = demoRow.itemRect(2, weights, 3);

  buttonA.onClick = []() { Serial.println("Button A tapped"); };
  buttonB.onClick = []() { Serial.println("Button B tapped"); };
  buttonC.onClick = []() { Serial.println("Button C tapped"); };
  app.screen().addWidget(buttonA);
  app.screen().addWidget(buttonB);
  app.screen().addWidget(buttonC);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
