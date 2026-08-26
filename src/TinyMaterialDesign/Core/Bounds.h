#pragma once
#include <stddef.h>
#include <stdint.h>

namespace tinymd {

/// Clamps a signed widget-layout coordinate to a non-negative pixel index
/// before it's passed into one of TinyGPU's size_t-based drawing calls.
/// Needed because Bounds (this file) is deliberately signed while ISurface
/// drawing calls take size_t - passing a negative value straight through
/// would silently wrap to a huge unsigned value instead of clipping.
inline size_t toPx(int32_t v) { return v < 0 ? 0 : static_cast<size_t>(v); }

/**
 * @brief Axis-aligned pixel rectangle used for widget bounds and hit-testing.
 *
 * Deliberately signed (unlike TinyGPU's size_t-based drawing calls) so
 * widget layout math (x - radius, centering, ...) can't silently underflow;
 * clamp to >=0 only at the point of calling into ISurface.
 */
struct Bounds {
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;

  Bounds() = default;
  Bounds(int32_t x, int32_t y, int32_t w, int32_t h) : x(x), y(y), w(w), h(h) {}

  int32_t right() const { return x + w; }
  int32_t bottom() const { return y + h; }
  int32_t centerX() const { return x + w / 2; }
  int32_t centerY() const { return y + h / 2; }

  bool contains(int32_t px, int32_t py) const {
    return px >= x && py >= y && px < right() && py < bottom();
  }

  /// Returns a rect inset by `amount` on every side (negative to grow).
  Bounds inset(int32_t amount) const {
    return Bounds(x + amount, y + amount, w - 2 * amount, h - 2 * amount);
  }
};

}  // namespace tinymd
