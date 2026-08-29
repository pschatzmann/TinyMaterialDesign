// Covers: a Container/Screen whose own content exactly fits (no overflow)
// must not swallow a pan/scroll gesture as a no-op self-scroll - it should
// forward it to a nested Container underneath that genuinely needs to
// scroll. Regression test for a bug found while building the 10,000-row
// virtualized-list example (docs/Tutorial.md's "Virtualized content"
// chapter): Screen's own content fit its viewport exactly, so it silently
// ate every drag before the nested scrollable list ever saw it.
#include <TinyMaterialDesign.h>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

int main() {
  Surface<RGB565> surface(240, 320, FontRGB565);
  surface.begin();
  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
  Screen<RGB565> screen(theme);

  // Screen's only child is `inner`, sized to exactly fill the remaining
  // viewport below a fixed app bar - so Screen's own contentHeight()
  // equals its own bounds.h (no overflow, no self-scroll).
  constexpr int32_t kAppBarHeight = 56;
  Button<RGB565> appBar(Bounds(0, 0, 240, kAppBarHeight), "Bar");
  screen.addFixedWidget(appBar);

  Container<RGB565> inner(Bounds(0, kAppBarHeight, 240, 320 - kAppBarHeight));
  constexpr int kRowHeight = 40;
  constexpr int kRowCount = 20;  // 800px of content in a 264px viewport
  Button<RGB565> rows[kRowCount] = {};
  for (int i = 0; i < kRowCount; ++i) {
    rows[i] = Button<RGB565>(Bounds(10, kAppBarHeight + i * kRowHeight, 220, kRowHeight), "Row");
    inner.addChild(rows[i]);
  }
  screen.addWidget(inner);

  screen.draw(surface);
  TMD_CHECK(screen.scrollOffset() == 0);  // Screen itself has nothing to scroll

  GestureEvent e;
  e.type = GestureType::kPan;
  e.phase = GesturePhase::kBegan;
  e.point = {120, 200};
  e.startPoint = e.point;
  screen.handleGesture(e);

  e.phase = GesturePhase::kChanged;
  e.stepDeltaY = -300;
  screen.handleGesture(e);

  e.phase = GesturePhase::kEnded;
  e.stepDeltaY = 0;
  screen.handleGesture(e);

  TMD_CHECK(screen.scrollOffset() == 0);  // still nothing for Screen itself to scroll
  TMD_CHECK(inner.scrollOffset() > 0);    // the drag reached the nested Container instead

  if (tmd_test_ok) printf("test_nested_scroll: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
