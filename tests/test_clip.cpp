// Covers: Container cropping a child that only partially overlaps its own
// bounds (see ISurface::pushClipRect(), added to TinyGPU and used by
// Container::drawChildren()) instead of drawing it in full or skipping it.
#include <TinyMaterialDesign.h>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

// A trivial leaf widget that fills its own bounds solid so we can check
// exactly which pixels actually got painted.
template <typename RGB_T>
class SolidBlock : public Widget<RGB_T> {
 public:
  explicit SolidBlock(Bounds b, RGB_T color) : color_(color) { this->bounds = b; }
  void draw(tinygpu::ISurface<RGB_T>& target) override {
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), color_);
  }

 private:
  RGB_T color_;
};

int main() {
  constexpr size_t kWidth = 100;
  constexpr size_t kHeight = 100;
  Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
  surface.begin();
  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
  Screen<RGB565> screen(theme);

  const RGB565 kBg(0, 0, 0);
  const RGB565 kFg(255, 255, 255);
  screen.setBackgroundColor(kBg);

  // Container occupies y in [20, 60). A child block spans y in [10, 80) -
  // well past both edges - so only the [20, 60) band should end up filled.
  Container<RGB565> box(Bounds(0, 20, 100, 40));
  SolidBlock<RGB565> block(Bounds(0, 10, 100, 70), kFg);
  box.addChild(block);
  screen.addWidget(box);

  screen.draw(surface);

  int litInBand = 0, litAboveBand = 0, litBelowBand = 0;
  for (size_t y = 0; y < kHeight; ++y) {
    const bool lit = surface.getPixel(0, y) != kBg;
    if (y >= 20 && y < 60) {
      if (lit) litInBand++;
    } else if (y < 20) {
      if (lit) litAboveBand++;
    } else {
      if (lit) litBelowBand++;
    }
  }

  TMD_CHECK(litInBand == 40);      // the whole visible band is filled
  TMD_CHECK(litAboveBand == 0);    // nothing painted above the container
  TMD_CHECK(litBelowBand == 0);    // nothing painted below the container

  if (tmd_test_ok) printf("test_clip: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
