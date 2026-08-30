/**
 * @file table-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `TableLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- TableLayout (the control under test) -----------------------------------
// Not a widget - a grid with independently-sized columns/rows, unlike
// GridLayout's uniform cells. A wide first column next to a narrow second
// one, three rows tall.
constexpr int32_t kColumnWidths[] = {150, 60};
constexpr int32_t kRowHeights[] = {70, 70, 70};
TableLayout demoTable(Bounds(10, 20, app.width() - 20, app.height() - 40), kColumnWidths, 2, kRowHeights, 3);

Card<RGB565> wide0(Bounds(), "Wide", "Row 0");
Card<RGB565> narrow0(Bounds(), "N", nullptr);
Card<RGB565> wide1(Bounds(), "Wide", "Row 1");
Card<RGB565> narrow1(Bounds(), "N", nullptr);
Card<RGB565> wide2(Bounds(), "Wide", "Row 2");
Card<RGB565> narrow2(Bounds(), "N", nullptr);

void setup() {
  Serial.begin(115200);
  app.begin();

  wide0.bounds = demoTable.cellRect(0, 0);
  narrow0.bounds = demoTable.cellRect(0, 1);
  wide1.bounds = demoTable.cellRect(1, 0);
  narrow1.bounds = demoTable.cellRect(1, 1);
  wide2.bounds = demoTable.cellRect(2, 0);
  narrow2.bounds = demoTable.cellRect(2, 1);

  app.screen().addWidget(wide0);
  app.screen().addWidget(narrow0);
  app.screen().addWidget(wide1);
  app.screen().addWidget(narrow1);
  app.screen().addWidget(wide2);
  app.screen().addWidget(narrow2);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
