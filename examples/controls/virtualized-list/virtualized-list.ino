/**
 * @file virtualized-list.ino
 * @brief Minimal, self-contained demo of `Container::setChildProvider()` -
 * a scrolling list of 10,000 logical rows served by a pool of only 12 real
 * `ListItem` widgets.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

constexpr int32_t kAppBarHeight = 56;
constexpr int32_t kRowHeight = 40;
constexpr int kLogicalCount = 10000;   // how many rows the list *reports*
constexpr size_t kPoolSize = 12;       // how many ListItem widgets actually exist

AppBar<RGB565> appBar(Bounds(0, 0, app.width(), kAppBarHeight), "10,000 Rows");

// --- Container + setChildProvider() (the control under test) ----------------
// A viewport the height of the remaining screen; content is 10,000 rows *
// 40px = 400,000px tall, scrolled the same way any other Container's
// content would be - setChildProvider() just means those rows aren't
// 10,000 real ListItem objects sitting in memory the whole time.
Container<RGB565> bigList(Bounds(0, kAppBarHeight, app.width(), app.height() - kAppBarHeight));

// The entire pool: whichever ~8 rows are on screen at once (plus a little
// slack) are drawn using these 12 objects, repositioned and relabeled for
// whatever index currently needs them. 10,000 real ListItems would cost
// roughly a megabyte (each one owns a std::string title + std::function
// callback); this pool costs a few hundred bytes, however large
// kLogicalCount grows.
ListItem<RGB565> pool[kPoolSize];

void setup() {
  Serial.begin(115200);
  app.begin();

  // O(1) content-height/scroll-range math instead of visiting all 10,000
  // logical rows on every frame - see Container::setUniformItemHeight().
  bigList.setUniformItemHeight(kRowHeight);

  bigList.setChildProvider(
      []() { return kLogicalCount; },
      [](int index) -> Widget<RGB565>& {
        ListItem<RGB565>& item = pool[static_cast<size_t>(index) % kPoolSize];
        item.bounds = Bounds(bigList.bounds.x, bigList.bounds.y + index * kRowHeight, bigList.bounds.w,
                             kRowHeight);
        char label[24];
        snprintf(label, sizeof(label), "Row %d", index);
        item.setTitle(label);
        item.onClick = [index]() { Serial.print("Row "); Serial.print(index); Serial.println(" tapped"); };
        return item;
      });

  app.screen().addFixedWidget(appBar);
  app.screen().addWidget(bigList);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
