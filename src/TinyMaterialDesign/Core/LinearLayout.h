#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

enum class LayoutAxis { Horizontal, Vertical };

/**
 * @brief Computes equal- or weighted-split rects along one axis of a container.
 *
 * Not a widget - see GridLayout.h for the rationale. Use it before
 * constructing/positioning widgets:
 *
 *   LinearLayout row(Bounds(16, 56, 308, 48), LayoutAxis::Horizontal);
 *   button1.bounds = row.itemRect(0, 3);
 *   button2.bounds = row.itemRect(1, 3);
 *   button3.bounds = row.itemRect(2, 3);
 *
 * By default all `count` items split the container equally (minus spacing
 * gutters). Pass per-item weights via itemRect(index, weights, count) to give
 * some items more of the remaining space than others (weights are relative,
 * not required to sum to 1).
 */
class LinearLayout {
 public:
  LinearLayout(Bounds container, LayoutAxis axis, int32_t spacing = 8)
      : container_(container), axis_(axis), spacing_(spacing) {}

  /// Bounds for the `index`-th of `count` equally-sized items (0-based).
  Bounds itemRect(int index, int count) const {
    if (count <= 0) count = 1;
    const int32_t total = axis_ == LayoutAxis::Horizontal ? container_.w : container_.h;
    const int32_t extent = (total - (count - 1) * spacing_) / count;
    return rectAt(index * (extent + spacing_), extent);
  }

  /// Bounds for the `index`-th of `count` items, sized proportionally to
  /// `weights[index]` relative to the sum of `weights[0..count)`.
  Bounds itemRect(int index, const float* weights, int count) const {
    if (count <= 0) return Bounds(container_.x, container_.y, 0, 0);
    float sum = 0;
    for (int i = 0; i < count; ++i) sum += weights[i];
    if (sum <= 0) return itemRect(index, count);

    const int32_t total = axis_ == LayoutAxis::Horizontal ? container_.w : container_.h;
    const int32_t available = total - (count - 1) * spacing_;

    int32_t offset = 0;
    for (int i = 0; i < index; ++i) offset += static_cast<int32_t>(available * (weights[i] / sum)) + spacing_;
    const int32_t extent = static_cast<int32_t>(available * (weights[index] / sum));
    return rectAt(offset, extent);
  }

 private:
  Bounds rectAt(int32_t offset, int32_t extent) const {
    if (axis_ == LayoutAxis::Horizontal) {
      return Bounds(container_.x + offset, container_.y, extent, container_.h);
    }
    return Bounds(container_.x, container_.y + offset, container_.w, extent);
  }

  Bounds container_;
  LayoutAxis axis_;
  int32_t spacing_;
};

}  // namespace tinymd
