/**
 * @file container.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Container`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- Container (the control under test) -------------------------------------
// A viewport shorter than its content, so it scrolls automatically; one of
// its children is itself a Container, showing that containers nest.
Container<RGB565> demoContainer(Bounds(0, 0, app.width(), 260));

Button<RGB565> buttonA(Bounds(20, 16, app.width() - 40, 48), "Button A");
Button<RGB565> buttonB(Bounds(20, 76, app.width() - 40, 48), "Button B");

Container<RGB565> nestedPanel(Bounds(20, 136, app.width() - 40, 120));
Card<RGB565> nestedCard(Bounds(0, 0, app.width() - 40, 100), "Nested",
                        "This card lives inside a Container that is itself a "
                        "child of demoContainer.");

Button<RGB565> buttonC(Bounds(20, 268, app.width() - 40, 48), "Button C (scroll to reach)");

void setup() {
  Serial.begin(115200);
  app.begin();

  buttonA.onClick = []() { Serial.println("Button A tapped"); };
  buttonB.onClick = []() { Serial.println("Button B tapped"); };
  buttonC.onClick = []() { Serial.println("Button C tapped"); };

  nestedPanel.addChild(nestedCard);

  demoContainer.addChild(buttonA);
  demoContainer.addChild(buttonB);
  demoContainer.addChild(nestedPanel);
  demoContainer.addChild(buttonC);

  app.screen().addWidget(demoContainer);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
