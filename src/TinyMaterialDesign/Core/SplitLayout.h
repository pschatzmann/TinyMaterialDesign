#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"
#include "TinyMaterialDesign/Core/LinearLayout.h"

namespace tinymd {

/**
 * @brief Splits a container into two panes (master/detail, nav/content).
 *
 * Not a widget - see GridLayout.h for the rationale.
 *
 *   SplitLayout split(Bounds(0, 56, 320, 400), LayoutAxis::Horizontal, 96);
 *   nav.bounds = split.firstRect();
 *   content.bounds = split.secondRect();
 *
 * The first pane gets `firstSize` pixels (along `axis`); the second pane
 * gets the remainder minus the gutter. Use ratio() instead of a fixed pixel
 * size to split proportionally (e.g. 0.3f for a 30/70 split).
 */
class SplitLayout {
 public:
  SplitLayout(Bounds container, LayoutAxis axis, int32_t firstSize, int32_t gutter = 8)
      : container_(container), axis_(axis), firstSize_(firstSize), gutter_(gutter) {}

  static SplitLayout ratio(Bounds container, LayoutAxis axis, float firstRatio, int32_t gutter = 8) {
    const int32_t total = axis == LayoutAxis::Horizontal ? container.w : container.h;
    return SplitLayout(container, axis, static_cast<int32_t>(total * firstRatio), gutter);
  }

  Bounds firstRect() const {
    if (axis_ == LayoutAxis::Horizontal) return Bounds(container_.x, container_.y, firstSize_, container_.h);
    return Bounds(container_.x, container_.y, container_.w, firstSize_);
  }

  Bounds secondRect() const {
    const int32_t offset = firstSize_ + gutter_;
    if (axis_ == LayoutAxis::Horizontal) {
      return Bounds(container_.x + offset, container_.y, container_.w - offset, container_.h);
    }
    return Bounds(container_.x, container_.y + offset, container_.w, container_.h - offset);
  }

 private:
  Bounds container_;
  LayoutAxis axis_;
  int32_t firstSize_;
  int32_t gutter_;
};

}  // namespace tinymd
