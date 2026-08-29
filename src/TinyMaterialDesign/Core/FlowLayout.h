#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/**
 * @brief Computes wrapping rects for items of varying width (chip/tag rows).
 *
 * Not a widget - see GridLayout.h for the rationale. Unlike GridLayout
 * (uniform cells), each item reports its own width; items are packed
 * left-to-right and wrap to a new row once they would overflow the
 * container's width:
 *
 *   FlowLayout flow(Bounds(16, 56, 308, 200));
 *   int32_t widths[] = {60, 90, 70, 100, 50};
 *   for (int i = 0; i < 5; ++i) chips[i].bounds = flow.next(widths[i], chipHeight);
 *
 * Calls to next() must be made in order (0, 1, 2, ...) since each call
 * advances internal cursor state. Use totalHeight() after a full pass, or
 * reset() to run the same set of widths again once you know it.
 */
class FlowLayout {
 public:
  explicit FlowLayout(Bounds container, int32_t spacing = 8)
      : container_(container), spacing_(spacing) {
    reset();
  }

  /// Bounds for the next item of the given `width`/`height`, wrapping to a
  /// new row if it doesn't fit in the remaining width of the current row.
  Bounds next(int32_t width, int32_t height) {
    if (cursorX_ != container_.x && cursorX_ + width > container_.right()) {
      cursorX_ = container_.x;
      cursorY_ += rowHeight_ + spacing_;
      rowHeight_ = 0;
    }
    const Bounds rect(cursorX_, cursorY_, width, height);
    cursorX_ += width + spacing_;
    if (height > rowHeight_) rowHeight_ = height;
    return rect;
  }

  /// Restarts packing from the container's top-left corner.
  void reset() {
    cursorX_ = container_.x;
    cursorY_ = container_.y;
    rowHeight_ = 0;
  }

  /// Total height consumed by items placed since the last reset().
  int32_t totalHeight() const { return (cursorY_ - container_.y) + rowHeight_; }

 private:
  Bounds container_;
  int32_t spacing_;
  int32_t cursorX_ = 0;
  int32_t cursorY_ = 0;
  int32_t rowHeight_ = 0;
};

}  // namespace tinymd
