/**
 * @file anchor-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `AnchorLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- AnchorLayout (the control under test) ----------------------------------
// Not a widget - positions a rect relative to a corner/edge of a container.
// The badge is anchored to backgroundCard's own top-right corner; the FAB is
// anchored to the whole screen's bottom-right corner.
Card<RGB565> backgroundCard(Bounds(10, 10, app.width() - 20, app.height() - 20), "AnchorLayout",
                            "Badge anchored to my corner; FAB anchored to the screen's.");
Badge<RGB565> cornerBadge(Bounds(), "3");
FAB demoFab(Bounds(), drawPlus<RGB565>);

void setup() {
  Serial.begin(115200);
  app.begin();

  AnchorLayout cardAnchor(backgroundCard.bounds);
  cornerBadge.bounds = cardAnchor.rect(Anchor::TopRight, 24, 24);

  AnchorLayout screenAnchor(Bounds(0, 0, app.width(), app.height()), /*margin=*/16);
  demoFab.bounds = screenAnchor.rect(Anchor::BottomRight, 56, 56);
  demoFab.onClick = []() { Serial.println("FAB tapped"); };

  app.screen().addWidget(backgroundCard);
  app.screen().addWidget(cornerBadge);
  app.screen().addWidget(demoFab);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
