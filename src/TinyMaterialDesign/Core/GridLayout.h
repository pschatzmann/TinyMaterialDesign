#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/**
 * @brief Computes wrapped grid cell positions for a row of equal-size cards.
 *
 * Not a widget - TinyMaterialDesign deliberately has no auto-layout
 * container (widgets always get an explicit Bounds), so this is a small
 * calculator you use before constructing/positioning widgets, the same way
 * Dialog::actionRect()/Drawer::itemRect() suggest single-widget spots:
 *
 *   GridLayout grid(Bounds(16, 56, 308, 400), 90, 110);
 *   for (size_t i = 0; i < cards.size(); ++i) cards[i].bounds = grid.cellRect(i);
 *
 * As many columns as fit `container`'s width are used; further items wrap
 * to additional rows. Rows are not clipped to the container's height - use
 * totalHeight() to size a scroll area or decide how many items actually fit.
 */
class GridLayout {
 public:
  GridLayout(Bounds container, int32_t cellWidth, int32_t cellHeight, int32_t spacing = 8)
      : container_(container), cellWidth_(cellWidth), cellHeight_(cellHeight), spacing_(spacing) {
    const int32_t step = cellWidth_ + spacing_;
    columns_ = step > 0 ? (container_.w + spacing_) / step : 1;
    if (columns_ < 1) columns_ = 1;
  }

  /// Bounds for the `index`-th cell (0-based, row-major).
  Bounds cellRect(int index) const {
    const int32_t col = index % columns_;
    const int32_t row = index / columns_;
    const int32_t x = container_.x + col * (cellWidth_ + spacing_);
    const int32_t y = container_.y + row * (cellHeight_ + spacing_);
    return Bounds(x, y, cellWidth_, cellHeight_);
  }

  int columns() const { return columns_; }

  /// Total height needed to lay out `count` items.
  int32_t totalHeight(int count) const {
    if (count <= 0) return 0;
    const int32_t rows = (count + columns_ - 1) / columns_;
    return rows * cellHeight_ + (rows - 1) * spacing_;
  }

 private:
  Bounds container_;
  int32_t cellWidth_;
  int32_t cellHeight_;
  int32_t spacing_;
  int32_t columns_;
};

}  // namespace tinymd
