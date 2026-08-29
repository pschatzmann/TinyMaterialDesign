// Covers: nested Container scrolling, gesture dispatch through two levels
// of nesting, and hit-testing after a scroll offset changes.
#include <TinyMaterialDesign.h>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

int main() {
  Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
  surface.begin();

  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
  Screen<RGB565> screen(theme);

  // Outer container: viewport smaller than its content -> should scroll.
  Container<RGB565> outer(Bounds(0, 0, kWidth, 200));

  Button<RGB565> btn1(Bounds(10, 0, 100, 40), "One");
  Button<RGB565> btn2(Bounds(10, 250, 100, 40), "Two (below fold)");
  int clicks1 = 0, clicks2 = 0;
  btn1.onClick = [&]() { clicks1++; };
  btn2.onClick = [&]() { clicks2++; };

  Container<RGB565> inner(Bounds(10, 60, 220, 100));
  Button<RGB565> nestedBtn(Bounds(0, 60, 100, 30), "Nested");
  int nestedClicks = 0;
  nestedBtn.onClick = [&]() { nestedClicks++; };
  inner.addChild(nestedBtn);

  outer.addChild(btn1);
  outer.addChild(inner);
  outer.addChild(btn2);

  screen.addWidget(outer);
  screen.setTheme(theme);
  screen.draw(surface);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {60, 20};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(clicks1 == 1);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 75};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(nestedClicks == 1);

  {
    GestureEvent e;
    e.type = GestureType::kPan;
    e.phase = GesturePhase::kBegan;
    e.point = {200, 45};
    e.startPoint = e.point;
    outer.onGesture(e);

    e.phase = GesturePhase::kChanged;
    e.stepDeltaY = -100;
    outer.onGesture(e);

    e.phase = GesturePhase::kEnded;
    e.stepDeltaY = 0;
    outer.onGesture(e);
  }
  TMD_CHECK(outer.scrollOffset() > 0);

  screen.draw(surface);
  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    int32_t screenY = 250 - outer.scrollOffset() + 10;
    e.point = {60, static_cast<int16_t>(screenY)};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(clicks2 == 1);

  if (tmd_test_ok) printf("test_container: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
