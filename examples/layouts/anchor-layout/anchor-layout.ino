/**
 * @file anchor-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `AnchorLayout`.
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

// --- AnchorLayout (the control under test) ----------------------------------
// Not a widget - positions a rect relative to a corner/edge of a container.
// The badge is anchored to backgroundCard's own top-right corner; the FAB is
// anchored to the whole screen's bottom-right corner.
Card<RGB565> backgroundCard(Bounds(10, 10, kWidth - 20, kHeight - 20), "AnchorLayout",
                            "Badge anchored to my corner; FAB anchored to the screen's.");
Badge<RGB565> cornerBadge(Bounds(), "3");
FAB demoFab(Bounds(), drawPlus<RGB565>);

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  AnchorLayout cardAnchor(backgroundCard.bounds);
  cornerBadge.bounds = cardAnchor.rect(Anchor::TopRight, 24, 24);

  AnchorLayout screenAnchor(Bounds(0, 0, kWidth, kHeight), /*margin=*/16);
  demoFab.bounds = screenAnchor.rect(Anchor::BottomRight, 56, 56);
  demoFab.onClick = []() { printf("FAB tapped\n"); };

  screen.addWidget(backgroundCard);
  screen.addWidget(cornerBadge);
  screen.addWidget(demoFab);

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
