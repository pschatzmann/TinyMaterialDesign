#pragma once
#include <algorithm>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// Horizontal progress bar. Determinate (value in [0,1]) or indeterminate
/// (a segment sweeps back and forth, driven by update()).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class LinearProgressIndicator : public Widget<RGB_T> {
 public:
  LinearProgressIndicator() = default;
  explicit LinearProgressIndicator(Bounds bounds, float value = 0.0f, bool indeterminate = false)
      : value_(clamp01(value)), indeterminate_(indeterminate) {
    this->bounds = bounds;
  }

  float value() const { return value_; }
  void setValue(float value) { value_ = clamp01(value); }
  void setIndeterminate(bool indeterminate) { indeterminate_ = indeterminate; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T trackColor = this->theme().colors.surfaceVariant;
    RGB_T activeColor = this->enabled ? this->theme().colors.primary
                                      : blend(this->theme().colors.primary, this->theme().colors.surface, 0.5f);
    const size_t radius = toPx(this->bounds.h / 2);

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, trackColor);

    if (indeterminate_) {
      constexpr float kSegmentFraction = 0.35f;
      float start = phase_ * (1.0f + kSegmentFraction) - kSegmentFraction;
      float end = start + kSegmentFraction;
      start = std::max(0.0f, start);
      end = std::min(1.0f, end);
      if (end > start) {
        const int32_t x0 = this->bounds.x + static_cast<int32_t>(start * this->bounds.w);
        const int32_t x1 = this->bounds.x + static_cast<int32_t>(end * this->bounds.w);
        target.fillRoundRect(toPx(x0), toPx(this->bounds.y), toPx(x1 - x0), toPx(this->bounds.h),
                             radius, activeColor);
      }
    } else if (value_ > 0.0f) {
      const int32_t width = static_cast<int32_t>(value_ * this->bounds.w);
      target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(width),
                           toPx(this->bounds.h), radius, activeColor);
    }
  }

  bool update(uint32_t nowMs) override {
    if (!indeterminate_) return false;
    if (!started_) {
      started_ = true;
      startMs_ = nowMs;
    }
    const uint32_t elapsed = (nowMs - startMs_) % kCycleMs;
    phase_ = static_cast<float>(elapsed) / static_cast<float>(kCycleMs);
    return true;
  }

 private:
  static constexpr uint32_t kCycleMs = 1200;

  float value_ = 0.0f;
  bool indeterminate_ = false;
  bool started_ = false;
  uint32_t startMs_ = 0;
  float phase_ = 0.0f;

  static float clamp01(float value) { return std::min(1.0f, std::max(0.0f, value)); }
};

/// Circular "spinner" ring. Determinate (value in [0,1], drawn as a sweep
/// starting at 12 o'clock) or indeterminate (a fixed-length arc rotates
/// continuously, driven by update()). Both use TinyGPU's ISurface::drawArc.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class CircularProgressIndicator : public Widget<RGB_T> {
 public:
  CircularProgressIndicator() = default;
  explicit CircularProgressIndicator(Bounds bounds, float value = 0.0f, bool indeterminate = false)
      : value_(clamp01(value)), indeterminate_(indeterminate) {
    this->bounds = bounds;
  }

  float value() const { return value_; }
  void setValue(float value) { value_ = clamp01(value); }
  void setIndeterminate(bool indeterminate) { indeterminate_ = indeterminate; }
  void setThickness(uint8_t thickness) { thickness_ = thickness; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T trackColor = this->theme().colors.surfaceVariant;
    RGB_T activeColor = this->enabled ? this->theme().colors.primary
                                      : blend(this->theme().colors.primary, this->theme().colors.surface, 0.5f);

    const int32_t diameter = this->bounds.w < this->bounds.h ? this->bounds.w : this->bounds.h;
    const size_t radius = toPx(diameter / 2 - thickness_ / 2);
    const size_t cx = toPx(this->bounds.centerX());
    const size_t cy = toPx(this->bounds.centerY());

    target.drawArc(cx, cy, radius, 0.0f, 359.9f, trackColor, thickness_);

    constexpr float kStartDeg = -90.0f;  // 12 o'clock
    if (indeterminate_) {
      constexpr float kSweepDeg = 110.0f;
      const float start = kStartDeg + phase_ * 360.0f;
      target.drawArc(cx, cy, radius, start, start + kSweepDeg, activeColor, thickness_);
    } else if (value_ > 0.0f) {
      target.drawArc(cx, cy, radius, kStartDeg, kStartDeg + value_ * 360.0f, activeColor,
                     thickness_);
    }
  }

  bool update(uint32_t nowMs) override {
    if (!indeterminate_) return false;
    if (!started_) {
      started_ = true;
      startMs_ = nowMs;
    }
    const uint32_t elapsed = (nowMs - startMs_) % kCycleMs;
    phase_ = static_cast<float>(elapsed) / static_cast<float>(kCycleMs);
    return true;
  }

 private:
  static constexpr uint32_t kCycleMs = 900;

  float value_ = 0.0f;
  bool indeterminate_ = false;
  uint8_t thickness_ = 4;
  bool started_ = false;
  uint32_t startMs_ = 0;
  float phase_ = 0.0f;

  static float clamp01(float value) { return std::min(1.0f, std::max(0.0f, value)); }
};

using LinearProgressIndicatorRGB565 = LinearProgressIndicator<tinygpu::RGB565>;
using LinearProgressIndicatorRGB666 = LinearProgressIndicator<tinygpu::RGB666>;
using LinearProgressIndicatorRGB888 = LinearProgressIndicator<tinygpu::RGB888>;
using CircularProgressIndicatorRGB565 = CircularProgressIndicator<tinygpu::RGB565>;
using CircularProgressIndicatorRGB666 = CircularProgressIndicator<tinygpu::RGB666>;
using CircularProgressIndicatorRGB888 = CircularProgressIndicator<tinygpu::RGB888>;

}  // namespace tinymd
