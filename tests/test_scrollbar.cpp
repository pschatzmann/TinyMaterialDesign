// Covers: tapping and dragging on a Container's scrollbar touch zone scrolls
// to the touched position; the zone only exists while the content overflows
// and has priority over children below it.
#include <TinyMaterialDesign.h>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

static void tap(Screen<RGB565>& screen, int16_t x, int16_t y) {
  GestureEvent e;
  e.type = GestureType::kTap;
  e.phase = GesturePhase::kEnded;
  e.point = {x, y};
  e.startPoint = e.point;
  screen.handleGesture(e);
}

int main() {
  Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
  surface.begin();
  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
  Screen<RGB565> screen(theme);

  // 200 px viewport at y=40 over 600 px of content -> maxScroll 400,
  // thumb height max(24, 200*200/600) = 66, track 134
  Container<RGB565> page(Bounds(0, 40, kWidth, 200));
  Button<RGB565> top(Bounds(8, 40, 200, 40), "Top");
  Button<RGB565> edge(Bounds(150, 100, 90, 40), "Edge");  // reaches into the touch zone
  Button<RGB565> bottom(Bounds(8, 600, 200, 40), "Bottom");
  int edgeClicks = 0;
  edge.onClick = [&]() { edgeClicks++; };
  page.addChild(top);
  page.addChild(edge);
  page.addChild(bottom);
  screen.addWidget(page);
  screen.draw(surface);  // sets the screen's bounds

  const int16_t barX = kWidth - 4;

  // tap at the bottom of the track -> scrolled to the end
  tap(screen, barX, 40 + 199);
  TMD_CHECK(page.scrollOffset() == 400);

  // tap at the top of the track -> back to the start
  tap(screen, barX, 40);
  TMD_CHECK(page.scrollOffset() == 0);

  // tap in the middle -> thumb centered there, i.e. about half way
  tap(screen, barX, 40 + 100);
  TMD_CHECK(page.scrollOffset() > 180 && page.scrollOffset() < 220);

  // the zone has priority over the child below it ...
  tap(screen, barX, 40);  // offset 0: "Edge" is at y 100..140, x 150..240
  tap(screen, kWidth - 2, 110);
  TMD_CHECK(edgeClicks == 0);
  // ... but the child still gets taps left of the zone (the zone tap above
  // scrolled the page, so go back to the top first)
  tap(screen, barX, 40);
  tap(screen, 160, 110);
  TMD_CHECK(edgeClicks == 1);

  // a drag starting on the scrollbar is classified as kDrag and the thumb
  // follows the finger
  TMD_CHECK(screen.isDraggableAt(barX, 60));
  TMD_CHECK(!screen.isDraggableAt(100, 60));
  {
    GestureEvent e;
    e.type = GestureType::kDrag;
    e.phase = GesturePhase::kBegan;
    e.startPoint = {barX, 50};
    e.point = e.startPoint;
    screen.handleGesture(e);
    e.phase = GesturePhase::kChanged;
    e.point = {barX, 40 + 199};
    screen.handleGesture(e);
    TMD_CHECK(page.scrollOffset() == 400);
    // moving off the bar keeps following (the drag is latched)
    e.point = {120, 40};
    screen.handleGesture(e);
    TMD_CHECK(page.scrollOffset() == 0);
    e.phase = GesturePhase::kEnded;
    screen.handleGesture(e);
  }

  // no overflow -> no touch zone: the tap reaches the child
  Screen<RGB565> screen2(theme);
  Container<RGB565> small(Bounds(0, 0, kWidth, 300));
  Button<RGB565> right(Bounds(150, 10, 90, 40), "Right");
  int rightClicks = 0;
  right.onClick = [&]() { rightClicks++; };
  small.addChild(right);
  screen2.addWidget(small);
  screen2.draw(surface);
  tap(screen2, barX, 20);
  TMD_CHECK(rightClicks == 1);
  TMD_CHECK(!screen2.isDraggableAt(barX, 20));

  return tmd_test_ok ? 0 : 1;
}
