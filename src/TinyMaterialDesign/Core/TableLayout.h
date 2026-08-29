#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/**
 * @brief Grid with explicit, independently-sized column widths and row heights.
 *
 * Not a widget - see GridLayout.h for the rationale. GridLayout assumes
 * uniform cells and computes the column count for you; TableLayout is for
 * dashboards mixing wide/narrow columns or short/tall rows, where you supply
 * the sizes explicitly:
 *
 *   int32_t colW[] = {120, 80, 80};
 *   int32_t rowH[] = {48, 96};
 *   TableLayout table(Bounds(16, 56, 280, 200), colW, 3, rowH, 2);
 *   cell.bounds = table.cellRect(row, col);
 */
class TableLayout {
 public:
  TableLayout(Bounds container, const int32_t* columnWidths, int columnCount, const int32_t* rowHeights,
              int rowCount, int32_t spacing = 8)
      : container_(container),
        columnWidths_(columnWidths),
        columnCount_(columnCount),
        rowHeights_(rowHeights),
        rowCount_(rowCount),
        spacing_(spacing) {}

  /// Bounds for the cell at (`row`, `col`), 0-based.
  Bounds cellRect(int row, int col) const {
    int32_t x = container_.x;
    for (int c = 0; c < col && c < columnCount_; ++c) x += columnWidths_[c] + spacing_;
    int32_t y = container_.y;
    for (int r = 0; r < row && r < rowCount_; ++r) y += rowHeights_[r] + spacing_;

    const int32_t w = col < columnCount_ ? columnWidths_[col] : 0;
    const int32_t h = row < rowCount_ ? rowHeights_[row] : 0;
    return Bounds(x, y, w, h);
  }

  int columns() const { return columnCount_; }
  int rows() const { return rowCount_; }

  /// Total width needed for all columns.
  int32_t totalWidth() const {
    int32_t total = 0;
    for (int c = 0; c < columnCount_; ++c) total += columnWidths_[c] + (c > 0 ? spacing_ : 0);
    return total;
  }

  /// Total height needed for all rows.
  int32_t totalHeight() const {
    int32_t total = 0;
    for (int r = 0; r < rowCount_; ++r) total += rowHeights_[r] + (r > 0 ? spacing_ : 0);
    return total;
  }

 private:
  Bounds container_;
  const int32_t* columnWidths_;
  int columnCount_;
  const int32_t* rowHeights_;
  int rowCount_;
  int32_t spacing_;
};

}  // namespace tinymd
