#pragma once
#include <math.h>

#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/**
 * @brief Places item rects evenly spaced around a circle.
 *
 * Not a widget - see GridLayout.h for the rationale. Useful for round
 * displays (watch-face style dials/menus) where LinearLayout/GridLayout's
 * rectilinear packing doesn't fit the screen shape:
 *
 *   RadialLayout dial(Bounds(0, 0, 240, 240), 90); // radius from center
 *   for (int i = 0; i < 6; ++i) items[i].bounds = dial.itemRect(i, 6, 36, 36);
 *
 * Angle 0 points up (12 o'clock) and items are placed clockwise, matching
 * how a clock/dial reads; pass a non-zero `startDegrees` to rotate the
 * whole ring.
 */
class RadialLayout {
 public:
  RadialLayout(Bounds container, int32_t radius, float startDegrees = 0.0f)
      : container_(container), radius_(radius), startDegrees_(startDegrees) {}

  /// Bounds for the `index`-th of `count` items, each `width` x `height`,
  /// centered on a point evenly spaced around the circle.
  Bounds itemRect(int index, int count, int32_t width, int32_t height) const {
    if (count <= 0) count = 1;
    const float degrees = startDegrees_ + 360.0f * static_cast<float>(index) / static_cast<float>(count);
    const float radians = degrees * (3.14159265358979323846f / 180.0f);
    const int32_t cx = container_.centerX() + static_cast<int32_t>(radius_ * sinf(radians));
    const int32_t cy = container_.centerY() - static_cast<int32_t>(radius_ * cosf(radians));
    return Bounds(cx - width / 2, cy - height / 2, width, height);
  }

 private:
  Bounds container_;
  int32_t radius_;
  float startDegrees_;
};

}  // namespace tinymd
