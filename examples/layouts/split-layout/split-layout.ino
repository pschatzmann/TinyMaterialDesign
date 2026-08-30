/**
 * @file split-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `SplitLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- SplitLayout (the control under test) -----------------------------------
// Not a widget - divides a container into two panes along one axis. Here a
// 35/65 horizontal split, e.g. a narrow nav pane next to a content pane.
Card<RGB565> navCard(Bounds(), "Nav", "35%");
Card<RGB565> contentCard(Bounds(), "Content", "The remaining 65%, minus the gutter.");

void setup() {
  Serial.begin(115200);
  app.begin();

  SplitLayout demoSplit =
      SplitLayout::ratio(Bounds(0, 20, app.width(), app.height() - 40), LayoutAxis::Horizontal, 0.35f);
  navCard.bounds = demoSplit.firstRect();
  contentCard.bounds = demoSplit.secondRect();

  app.screen().addWidget(navCard);
  app.screen().addWidget(contentCard);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
