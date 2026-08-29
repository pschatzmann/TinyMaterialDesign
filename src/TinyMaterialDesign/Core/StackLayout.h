#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/**
 * @brief Two unrelated single-rect placement helpers bundled under one name -
 * not "arrange these into a row/column" (that's LinearLayout), but "place
 * one rect relative to another rect it sits on top of or overlaps":
 *
 * - centered(w, h): centers a rect inside the container - e.g. an icon
 *   centered inside a button.
 * - offset(index, w, h): places the `index`-th of a series of same-size
 *   rects, each shifted `overlapStep` pixels from the previous one,
 *   starting at the container's (x, y) origin - e.g. a row of overlapping
 *   avatar chips. Only that origin is used - the container's width/height
 *   are never read, so pass whatever Bounds is convenient (even 0x0) as
 *   long as its x/y is where the series should start.
 *
 * Not a widget - see GridLayout.h for the rationale. A single StackLayout
 * is normally used for just one of the two, not both.
 *
 *   StackLayout stack(button.bounds);
 *   icon.bounds = stack.centered(20, 20);
 *
 *   StackLayout avatars(Bounds(16, 16, 0, 0), 20);  // 20px overlap step
 *   for (int i = 0; i < 3; ++i) avatar[i].bounds = avatars.offset(i, 32, 32);
 */
class StackLayout {
 public:
  explicit StackLayout(Bounds container, int32_t overlapStep = 0)
      : container_(container), overlapStep_(overlapStep) {}

  /// A `width` x `height` rect centered within the container.
  Bounds centered(int32_t width, int32_t height) const {
    return Bounds(container_.centerX() - width / 2, container_.centerY() - height / 2, width, height);
  }

  /// The `index`-th rect, offset horizontally by `index * overlapStep` from
  /// the container's origin - e.g. a row of overlapping avatars.
  Bounds offset(int index, int32_t width, int32_t height) const {
    return Bounds(container_.x + index * overlapStep_, container_.y, width, height);
  }

 private:
  Bounds container_;
  int32_t overlapStep_;
};

}  // namespace tinymd
