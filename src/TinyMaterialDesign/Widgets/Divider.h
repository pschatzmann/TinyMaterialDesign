#pragma once
#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// A themed hairline. Orientation follows the bounds' aspect ratio: wider
/// than tall draws horizontal (centered vertically), taller draws vertical
/// (centered horizontally).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Divider : public Widget<RGB_T> {
 public:
  Divider() = default;
  explicit Divider(Bounds bounds) { this->bounds = bounds; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const RGB_T color = this->theme().colors.outline;
    if (this->bounds.w >= this->bounds.h) {
      const int32_t y = this->bounds.centerY();
      target.drawLine(toPx(this->bounds.x), toPx(y), toPx(this->bounds.right() - 1), toPx(y),
                      color);
    } else {
      const int32_t x = this->bounds.centerX();
      target.drawLine(toPx(x), toPx(this->bounds.y), toPx(x), toPx(this->bounds.bottom() - 1),
                      color);
    }
  }
};

using DividerRGB565 = Divider<tinygpu::RGB565>;
using DividerRGB666 = Divider<tinygpu::RGB666>;
using DividerRGB888 = Divider<tinygpu::RGB888>;

}  // namespace tinymd
