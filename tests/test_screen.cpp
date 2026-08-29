// Covers: Screen's fixed-widget layer, its own scrollable content (now
// inherited from Container), a Container nested inside that content,
// dialog-modal gesture interception, and isDraggableAt().
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

  Button<RGB565> fixedBtn(Bounds(10, 0, 100, 40), "Fixed");
  int fixedClicks = 0;
  fixedBtn.onClick = [&]() { fixedClicks++; };
  screen.addFixedWidget(fixedBtn);

  Button<RGB565> scrollBtn1(Bounds(10, 60, 100, 40), "Scroll1");
  Button<RGB565> scrollBtn2(Bounds(10, 500, 100, 40), "Scroll2 (below fold)");
  int s1 = 0, s2 = 0;
  scrollBtn1.onClick = [&]() { s1++; };
  scrollBtn2.onClick = [&]() { s2++; };
  screen.addWidget(scrollBtn1);
  screen.addWidget(scrollBtn2);

  Container<RGB565> nested(Bounds(10, 120, 220, 100));
  Button<RGB565> nestedBtn(Bounds(0, 120, 100, 30), "Nested");
  int nestedClicks = 0;
  nestedBtn.onClick = [&]() { nestedClicks++; };
  nested.addChild(nestedBtn);
  screen.addWidget(nested);

  screen.draw(surface);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 20};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(fixedClicks == 1);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 80};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(s1 == 1);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 135};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(nestedClicks == 1);

  {
    GestureEvent e;
    e.type = GestureType::kPan;
    e.phase = GesturePhase::kBegan;
    e.point = {200, 300};
    e.startPoint = e.point;
    screen.handleGesture(e);

    e.phase = GesturePhase::kChanged;
    e.stepDeltaY = -300;
    screen.handleGesture(e);

    e.phase = GesturePhase::kEnded;
    e.stepDeltaY = 0;
    screen.handleGesture(e);
  }
  TMD_CHECK(screen.scrollOffset() > 0);

  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 20};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(fixedClicks == 2);

  screen.draw(surface);
  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    int32_t screenY = 500 - screen.scrollOffset() + 10;
    e.point = {60, static_cast<int16_t>(screenY)};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(s2 == 1);

  Dialog<RGB565> dialog(Bounds(20, 100, 200, 120), "Title", "Message");
  screen.presentDialog(dialog);
  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 20};  // would hit fixedBtn if not for the modal
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(fixedClicks == 2);  // unchanged - dialog intercepted everything
  screen.dismissDialog();

  TMD_CHECK(screen.isDraggableAt(50, 20) == false);

  // A Slider nested inside a Container added to Screen must be recognized
  // as draggable through the recursive isDraggableAt() path. Uses a fresh
  // Screen so it isn't affected by the scroll offset built up above.
  Screen<RGB565> dragScreen(theme);
  Container<RGB565> sliderHolder(Bounds(10, 0, 200, 60));
  Slider<RGB565> slider(Bounds(0, 0, 180, 40));
  sliderHolder.addChild(slider);
  dragScreen.addWidget(sliderHolder);
  dragScreen.draw(surface);
  TMD_CHECK(dragScreen.isDraggableAt(20, 10) == true);

  if (tmd_test_ok) printf("test_screen: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
