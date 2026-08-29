/**
 * @file container.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `Container`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoards.h>
#include <cstdio>

constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

#ifdef ESP32
LCDBoardGuitionESP32_LVGL_2_4Display board;
#else
LCDBoardDesktopSDL board(kWidth, kHeight);
#endif
Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
Screen<RGB565> screen(theme);

// --- Container (the control under test) -------------------------------------
// A viewport shorter than its content, so it scrolls automatically; one of
// its children is itself a Container, showing that containers nest.
Container<RGB565> demoContainer(Bounds(0, 0, kWidth, 260));

Button<RGB565> buttonA(Bounds(20, 16, kWidth - 40, 48), "Button A");
Button<RGB565> buttonB(Bounds(20, 76, kWidth - 40, 48), "Button B");

Container<RGB565> nestedPanel(Bounds(20, 136, kWidth - 40, 120));
Card<RGB565> nestedCard(Bounds(0, 0, kWidth - 40, 100), "Nested",
                        "This card lives inside a Container that is itself a "
                        "child of demoContainer.");

Button<RGB565> buttonC(Bounds(20, 268, kWidth - 40, 48), "Button C (scroll to reach)");

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  buttonA.onClick = []() { printf("Button A tapped\n"); };
  buttonB.onClick = []() { printf("Button B tapped\n"); };
  buttonC.onClick = []() { printf("Button C tapped\n"); };

  nestedPanel.addChild(nestedCard);

  demoContainer.addChild(buttonA);
  demoContainer.addChild(buttonB);
  demoContainer.addChild(nestedPanel);
  demoContainer.addChild(buttonC);

  screen.addWidget(demoContainer);

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) { return screen.isDraggableAt(x, y); };
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());
  // Screen tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  if (screen.isDirty()) {
    screen.draw(surface);
    display.writeData(surface);
  }
}
