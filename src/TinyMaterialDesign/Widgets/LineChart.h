#pragma once
#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// One data series of a LineChart.
template <typename RGB_T>
struct ChartSeries {
  /// The values, oldest/leftmost first. NaN values break the line.
  std::vector<float> values;
  RGB_T color;
  /// Line width in pixels.
  uint8_t width = 2;
  bool visible = true;
  /// Draw the line dashed (e.g. to tell apart series of similar colors).
  bool dashed = false;
  /// Name in the legend; series without a label are not listed.
  std::string label;
};

/// A horizontal line at a fixed value, e.g. a threshold or a target.
template <typename RGB_T>
struct ChartReferenceLine {
  float value = 0.0f;
  RGB_T color;
  bool dashed = true;
};

/**
 * @brief Line chart with one or more series, a vertical value scale with
 * round-numbered grid lines, optional reference lines, and an optional card
 * frame with a title and a footer line.
 *
 *   LineChart<RGB565> chart(Bounds(8, 8, 224, 140), "Temperature");
 *   int s = chart.addSeries(colorFromHex<RGB565>(0x06B6D4));
 *   chart.addReferenceLine(25.0f, colorFromHex<RGB565>(0xF59E0B));
 *   chart.setValues(s, readings);   // std::vector<float>
 *   screen.invalidate();            // data changed outside a gesture
 *
 * Y axis: automatic (the range of all visible values, snapped outward to a
 * 1/2/5 x 10^n step - see setGridCount()) or fixed via setYRange(); the grid
 * values can also be given explicitly via setGridValues().
 *
 * X axis: by default the points of each series are stretched over the full
 * plot width. setVisiblePoints(n) instead gives every point a fixed spacing
 * of width / (n - 1) with the newest point at the right edge - a rolling
 * "oscilloscope" trace that scrolls in from the right as points are added.
 *
 * Series with more points than the plot has pixel columns are drawn as a
 * min/max envelope per column, so peaks are never lost however many points
 * there are. All drawing goes through clipped primitives, so the chart works
 * inside a scrolling Container.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class LineChart : public Widget<RGB_T> {
 public:
  LineChart() = default;
  explicit LineChart(Bounds bounds, const char* title = nullptr) {
    this->bounds = bounds;
    if (title != nullptr) title_ = title;
  }

  /// Draws a rounded card behind the chart (default) - set to false to draw
  /// the chart directly on the screen background.
  bool card = true;

  void setTitle(const char* title) { title_ = title; }
  /// One line of text below the plot, e.g. a summary of the values.
  void setFooter(const char* footer) { footer_ = footer; }

  /// Adds a series and returns its index.
  int addSeries(RGB_T color, uint8_t width = 2) {
    ChartSeries<RGB_T> s;
    s.color = color;
    s.width = width;
    series_.push_back(s);
    return static_cast<int>(series_.size()) - 1;
  }

  int seriesCount() const { return static_cast<int>(series_.size()); }

  /// Name of the series in the legend (drawn above the footer when at least
  /// one series has a label).
  void setSeriesLabel(int index, const char* label) { series(index).label = label != nullptr ? label : ""; }
  /// Draw the series dashed.
  void setSeriesDashed(int index, bool dashed) { series(index).dashed = dashed; }
  ChartSeries<RGB_T>& series(int index) { return series_[static_cast<size_t>(index)]; }

  /// Replaces the values of the series `index`.
  void setValues(int index, const std::vector<float>& values) { series(index).values = values; }
  void setValues(int index, const float* values, size_t count) {
    series(index).values.assign(values, values + count);
  }

  /// Appends one value to the series `index`, dropping the oldest value once
  /// the series holds `maxPoints` values (0 = unlimited).
  void addValue(int index, float value, size_t maxPoints = 0) {
    std::vector<float>& v = series(index).values;
    if (maxPoints > 0 && v.size() >= maxPoints) v.erase(v.begin(), v.begin() + (v.size() - maxPoints + 1));
    v.push_back(value);
  }

  /// Adds a horizontal reference line and returns its index.
  int addReferenceLine(float value, RGB_T color, bool dashed = true) {
    ChartReferenceLine<RGB_T> line;
    line.value = value;
    line.color = color;
    line.dashed = dashed;
    lines_.push_back(line);
    return static_cast<int>(lines_.size()) - 1;
  }
  void setReferenceLine(int index, float value) { lines_[static_cast<size_t>(index)].value = value; }

  /// Fixes the value range of the Y axis.
  void setYRange(float minValue, float maxValue) {
    fixedRange_ = true;
    minY_ = minValue;
    maxY_ = maxValue;
  }
  /// Goes back to an automatic Y range.
  void clearYRange() { fixedRange_ = false; }

  /// Approximate number of grid lines of the automatic scale (default 4).
  void setGridCount(int count) { gridCount_ = std::max(1, count); }
  /// Uses exactly these grid values (labelled) instead of the automatic ones.
  void setGridValues(const std::vector<float>& values) { gridValues_ = values; }

  /// Fixed spacing for `count` points with the newest at the right edge (see
  /// the class comment); 0 stretches all points over the width (default).
  void setVisiblePoints(int count) { visiblePoints_ = count; }

  /// Formats a value of an axis label.
  using AxisFormatter = std::function<void(float value, char* buf, size_t len)>;

  /**
   * Labels the X axis: the left edge of the plot is `first`, the right edge
   * `last`, with `ticks` (>= 2) evenly spaced labels and tick marks including
   * both edges. The values only label the axis - the points are still placed
   * as described in the class comment. Example for a rolling minute:
   *   chart.setXAxis(-60, 0, 4, [](float v, char* b, size_t n) {
   *     snprintf(b, n, v == 0 ? "now" : "%.0fs", v);
   *   });
   */
  void setXAxis(float first, float last, int ticks = 3, AxisFormatter format = nullptr) {
    xAxis_ = true;
    xFirst_ = first;
    xLast_ = last;
    xTicks_ = std::max(2, ticks);
    xFormat_ = std::move(format);
  }
  /// Removes the X axis labels.
  void clearXAxis() { xAxis_ = false; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    const RGB_T background = card ? t.colors.surface : t.colors.background;
    if (card) {
      const size_t radius = toPx(t.shape.large);
      target.fillRoundRect(toPx(b.x), toPx(b.y), toPx(b.w), toPx(b.h), radius, t.colors.surface);
      target.drawRoundRect(toPx(b.x), toPx(b.y), toPx(b.w), toPx(b.h), radius, t.colors.outline);
    }
    const int32_t pad = card ? 8 : 0;
    tinygpu::IFont<RGB_T>& small = *t.typography.label;
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    int32_t top = b.y + (card ? 6 : 0);
    if (!title_.empty()) {
      text(target, body, b.x + pad, top, title_.c_str(), t.colors.onSurface, background);
      top += static_cast<int32_t>(body.getHeight(1)) + 8;
    } else {
      top += 4;
    }
    int32_t bottom = b.bottom() - (card ? 6 : 0);
    if (!footer_.empty()) {
      bottom -= static_cast<int32_t>(small.getHeight(1));
      text(target, small, b.x + pad, bottom, footer_.c_str(), t.colors.onSurfaceVariant, background);
      bottom -= 6;
    } else {
      bottom -= 4;
    }
    if (hasLegend()) {
      bottom -= static_cast<int32_t>(small.getHeight(1));
      drawLegend(target, small, b.x + pad, b.right() - pad, bottom, background);
      bottom -= 6;
    }
    if (xAxis_) bottom -= static_cast<int32_t>(small.getHeight(1)) + 6;  // room for the labels

    // value range
    double lo = minY_, hi = maxY_;
    if (!fixedRange_) {
      lo = INFINITY;
      hi = -INFINITY;
      for (const ChartSeries<RGB_T>& s : series_) {
        if (!s.visible) continue;
        for (float v : s.values) {
          if (!isfinite(v)) continue;
          lo = std::min<double>(lo, v);
          hi = std::max<double>(hi, v);
        }
      }
      if (!isfinite(lo)) {
        lo = 0.0;
        hi = 1.0;
      }
    }
    if (hi - lo < 1e-9) {
      lo -= 0.5;
      hi += 0.5;
    }

    // grid values and their labels
    std::vector<double> grid;
    double step = 0;
    if (!gridValues_.empty()) {
      for (float v : gridValues_) grid.push_back(v);
      step = grid.size() > 1 ? fabs(grid[1] - grid[0]) : hi - lo;
    } else {
      step = niceStep(hi - lo, gridCount_);
      if (!fixedRange_) {
        lo = floor(lo / step) * step;
        hi = ceil(hi / step) * step;
      }
      for (double v = ceil(lo / step - 1e-9) * step; v <= hi + step * 1e-6; v += step) grid.push_back(v);
    }
    if (step <= 0) step = 1;
    int decimals = static_cast<int>(-floor(log10(step)));
    decimals = std::min(4, std::max(0, decimals));
    char buf[24];
    int32_t labelW = 0;
    for (double v : grid) {
      label(buf, sizeof(buf), v, step, decimals);
      labelW = std::max<int32_t>(labelW, static_cast<int32_t>(small.measureTextWidth(buf)));
    }

    const int32_t left = b.x + pad + labelW + 4;
    const int32_t right = b.right() - pad;
    const int32_t plotW = right - left;
    const int32_t plotH = bottom - top;
    if (plotW < 4 || plotH < 4) return;
    auto yOf = [&](double v) {
      v = std::min(std::max(v, lo), hi);
      return top + plotH - static_cast<int32_t>((v - lo) / (hi - lo) * plotH);
    };

    for (double v : grid) {
      if (v < lo - step * 1e-6 || v > hi + step * 1e-6) continue;
      const int32_t y = yOf(v);
      if (fabs(v) < step * 1e-6) {
        target.fillRect(toPx(left), toPx(y), toPx(plotW), 1, t.colors.onSurfaceVariant);
      } else {
        dashedLine(target, left, right, y, t.colors.outline, 3, 3);
      }
      label(buf, sizeof(buf), v, step, decimals);
      text(target, small, left - 4 - static_cast<int32_t>(small.measureTextWidth(buf)),
           y - static_cast<int32_t>(small.getHeight(1)) / 2, buf, t.colors.onSurfaceVariant, background);
    }
    target.fillRect(toPx(left), toPx(top), 1, toPx(plotH), t.colors.outline);
    if (xAxis_) drawXAxis(target, small, left, right, top + plotH, background);

    for (const ChartSeries<RGB_T>& s : series_) {
      if (s.visible) drawSeries(target, s, left, right, yOf);
    }
    for (const ChartReferenceLine<RGB_T>& line : lines_) {
      const int32_t y = yOf(line.value);
      if (line.dashed) {
        dashedLine(target, left, right, y, line.color, 6, 4);
        dashedLine(target, left, right, y + 1, line.color, 6, 4);
      } else {
        target.fillRect(toPx(left), toPx(y), toPx(plotW), 2, line.color);
      }
    }
  }

 private:
  std::string title_;
  std::string footer_;
  std::vector<ChartSeries<RGB_T>> series_;
  std::vector<ChartReferenceLine<RGB_T>> lines_;
  std::vector<float> gridValues_;
  bool fixedRange_ = false;
  float minY_ = 0.0f;
  float maxY_ = 1.0f;
  int gridCount_ = 4;
  int visiblePoints_ = 0;
  bool xAxis_ = false;
  float xFirst_ = 0, xLast_ = 0;
  int xTicks_ = 3;
  AxisFormatter xFormat_;

  /// One pixel column of a series: the first/last/min/max y of its points.
  struct Column {
    int32_t x = 0, first = 0, last = 0, min = 0, max = 0;
  };

  /// Tick marks and labels below the plot (y = bottom of the plot).
  void drawXAxis(tinygpu::ISurface<RGB_T>& target, tinygpu::IFont<RGB_T>& font, int32_t left, int32_t right,
                 int32_t y, RGB_T background) {
    const MaterialTheme<RGB_T>& t = this->theme();
    char buf[16];
    for (int i = 0; i < xTicks_; ++i) {
      const int32_t x = left + static_cast<int32_t>(static_cast<int64_t>(right - left) * i / (xTicks_ - 1));
      target.fillRect(toPx(x), toPx(y), 1, 3, t.colors.outline);
      const float v = xFirst_ + (xLast_ - xFirst_) * i / (xTicks_ - 1);
      if (xFormat_) {
        xFormat_(v, buf, sizeof(buf));
      } else {
        snprintf(buf, sizeof(buf), "%g", v);
      }
      const int32_t w = static_cast<int32_t>(font.measureTextWidth(buf));
      // the outer labels stay inside the plot, the others are centered
      const int32_t lx = i == 0 ? x : i == xTicks_ - 1 ? x - w : x - w / 2;
      text(target, font, lx, y + 4, buf, t.colors.onSurfaceVariant, background);
    }
  }

  bool hasLegend() const {
    for (const ChartSeries<RGB_T>& s : series_) {
      if (!s.label.empty()) return true;
    }
    return false;
  }

  /// One line: a short (dashed) stroke in the color of each series and its
  /// label, as many as fit.
  void drawLegend(tinygpu::ISurface<RGB_T>& target, tinygpu::IFont<RGB_T>& font, int32_t x, int32_t right,
                  int32_t y, RGB_T background) {
    const int32_t mid = y + static_cast<int32_t>(font.getHeight(1)) / 2;
    for (const ChartSeries<RGB_T>& s : series_) {
      if (s.label.empty() || !s.visible) continue;
      const int32_t w = 14 + 4 + static_cast<int32_t>(font.measureTextWidth(s.label.c_str()));
      if (x + w > right) break;
      if (s.dashed) {
        dashedLine(target, x, x + 14, mid, s.color, 4, 2);
        dashedLine(target, x, x + 14, mid + 1, s.color, 4, 2);
      } else {
        target.fillRect(toPx(x), toPx(mid), 14, 2, s.color);
      }
      text(target, font, x + 18, y, s.label.c_str(), s.color, background);
      x += w + 10;
    }
  }

  template <typename YOf>
  void drawSeries(tinygpu::ISurface<RGB_T>& target, const ChartSeries<RGB_T>& s, int32_t left, int32_t right,
                  YOf yOf) {
    const size_t n = s.values.size();
    if (n == 0) return;
    const int32_t plotW = right - left;
    bool pen = false;    // a previous column exists that the line continues from
    bool open = false;   // `col` holds data
    Column col, prev;
    int32_t dashCounter = 0;
    int32_t* dash = s.dashed ? &dashCounter : nullptr;
    auto flush = [&]() {
      if (!open) return;
      if (col.max > col.min) line(target, col.x, col.min, col.x, col.max, s.color, s.width, dash);
      if (pen) {
        line(target, prev.x, prev.last, col.x, col.first, s.color, s.width, dash);
      } else if (col.max == col.min) {
        line(target, col.x, col.first, col.x, col.first, s.color, s.width, dash);
      }
      prev = col;
      pen = true;
      open = false;
    };
    for (size_t i = 0; i < n; ++i) {
      int32_t x;
      if (visiblePoints_ > 1) {
        x = right - static_cast<int32_t>(static_cast<double>(n - 1 - i) * plotW / (visiblePoints_ - 1));
        if (x < left) continue;
      } else {
        x = n > 1 ? left + static_cast<int32_t>(static_cast<double>(plotW) * i / (n - 1)) : left;
      }
      const float v = s.values[i];
      if (!isfinite(v)) {
        flush();
        pen = false;
        continue;
      }
      const int32_t y = yOf(v);
      if (open && x != col.x) flush();
      if (!open) {
        col.x = x;
        col.first = col.last = col.min = col.max = y;
        open = true;
      } else {
        col.last = y;
        col.min = std::min(col.min, y);
        col.max = std::max(col.max, y);
      }
    }
    flush();
  }

  /// Bresenham line drawn with fillRect, so it honors the clip rectangle.
  /// With `dash`, only 5 of every 9 pixels are drawn (the counter continues
  /// over the segments of a series, so the pattern does not restart).
  static void line(tinygpu::ISurface<RGB_T>& target, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   RGB_T color, uint8_t width, int32_t* dash = nullptr) {
    const int32_t w = width == 0 ? 1 : width;
    const int32_t half = w / 2;
    const int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    while (true) {
      const bool on = dash == nullptr || ((*dash)++ % 9) < 5;
      if (on && x0 - half >= 0 && y0 - half >= 0) target.fillRect(toPx(x0 - half), toPx(y0 - half), toPx(w), toPx(w), color);
      if (x0 == x1 && y0 == y1) break;
      const int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  static void dashedLine(tinygpu::ISurface<RGB_T>& target, int32_t x0, int32_t x1, int32_t y, RGB_T color,
                         int32_t dash, int32_t gap) {
    for (int32_t x = x0; x < x1; x += dash + gap) {
      const int32_t end = std::min(x + dash, x1);
      target.fillRect(toPx(x), toPx(y), toPx(end - x), 1, color);
    }
  }

  static void text(tinygpu::ISurface<RGB_T>& target, tinygpu::IFont<RGB_T>& font, int32_t x, int32_t y,
                   const char* str, RGB_T color, RGB_T background) {
    font.drawText(target, static_cast<int16_t>(x), static_cast<int16_t>(y), str, color, background, false);
  }

  /// Round step (1, 2, 5 x 10^n) giving about `targetTicks` grid lines.
  static double niceStep(double range, int targetTicks) {
    const double raw = range / targetTicks;
    const double mag = pow(10.0, floor(log10(raw)));
    const double norm = raw / mag;
    const double nice = norm < 1.5 ? 1.0 : norm < 3.0 ? 2.0 : norm < 7.0 ? 5.0 : 10.0;
    return nice * mag;
  }

  /// Axis label; large scales are abbreviated (500k, 6M).
  static void label(char* buf, size_t len, double v, double step, int decimals) {
    if (fabs(v) < step * 1e-6) {
      snprintf(buf, len, "0");
    } else if (step >= 1e6) {
      snprintf(buf, len, "%gM", v / 1e6);
    } else if (step >= 1e3) {
      snprintf(buf, len, "%gk", v / 1e3);
    } else {
      snprintf(buf, len, "%.*f", decimals, v);
    }
  }
};

using LineChartRGB565 = LineChart<tinygpu::RGB565>;
using LineChartRGB666 = LineChart<tinygpu::RGB666>;
using LineChartRGB888 = LineChart<tinygpu::RGB888>;

}  // namespace tinymd
