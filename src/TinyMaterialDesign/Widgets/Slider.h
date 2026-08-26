#pragma once
#include <algorithm>
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Draggable track-and-thumb slider over [minValue, maxValue].
 *
 * Reports isDraggable() == true so Screen::isDraggableAt() (wired to
 * GestureDetector::isDraggable) classifies a press-and-move starting on the
 * slider as GestureType::kDrag rather than kPan/kScroll - and Screen latches
 * the drag to this widget for its whole lifetime, so the thumb keeps
 * tracking the finger even past the slider's own bounds. Tapping anywhere
 * on the track also jumps the thumb there.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Slider : public Widget<RGB_T> {
 public:
  Slider() = default;
  Slider(Bounds bounds, float minValue = 0.0f, float maxValue = 1.0f, float value = 0.0f)
      : minValue_(minValue), maxValue_(maxValue), value_(value) {
    this->bounds = bounds;
    value_ = clampValue(value_);
  }

  std::function<void(float)> onChange;

  float value() const { return value_; }
  void setValue(float value) { value_ = clampValue(value); }

  bool isDraggable() const override { return true; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    RGB_T trackColor = theme.colors.surfaceVariant;
    RGB_T activeColor = theme.colors.primary;
    RGB_T thumbColor = theme.colors.primary;
    if (!this->enabled) {
      trackColor = blend(trackColor, theme.colors.surface, 0.5f);
      activeColor = blend(activeColor, theme.colors.surface, 0.5f);
      thumbColor = blend(thumbColor, theme.colors.surface, 0.4f);
    }

    const int32_t trackHeight = 4;
    const int32_t trackTop = this->bounds.centerY() - trackHeight / 2;
    const size_t trackRadius = toPx(trackHeight / 2);

    target.fillRoundRect(toPx(this->bounds.x), toPx(trackTop), toPx(this->bounds.w),
                         toPx(trackHeight), trackRadius, trackColor);

    const int32_t thumbX = thumbCenterX();
    const int32_t activeWidth = thumbX - this->bounds.x;
    if (activeWidth > 0) {
      target.fillRoundRect(toPx(this->bounds.x), toPx(trackTop), toPx(activeWidth),
                           toPx(trackHeight), trackRadius, activeColor);
    }

    const int32_t thumbRadius = this->bounds.h / 2;
    target.fillCircle(toPx(thumbX), toPx(this->bounds.centerY()), toPx(thumbRadius), thumbColor);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type) && event.type != tinygpu::GestureType::kDrag) {
      return false;
    }
    const float previous = value_;
    setValueFromX(event.point.x);
    if (onChange && value_ != previous) onChange(value_);
    return true;
  }

 private:
  float minValue_ = 0.0f;
  float maxValue_ = 1.0f;
  float value_ = 0.0f;

  float clampValue(float value) const {
    return std::min(maxValue_, std::max(minValue_, value));
  }

  int32_t thumbCenterX() const {
    const float t = (maxValue_ > minValue_) ? (value_ - minValue_) / (maxValue_ - minValue_) : 0.0f;
    return this->bounds.x + static_cast<int32_t>(t * static_cast<float>(this->bounds.w));
  }

  void setValueFromX(int32_t x) {
    float t = this->bounds.w > 0
                  ? static_cast<float>(x - this->bounds.x) / static_cast<float>(this->bounds.w)
                  : 0.0f;
    t = std::min(1.0f, std::max(0.0f, t));
    value_ = clampValue(minValue_ + t * (maxValue_ - minValue_));
  }
};

using SliderRGB565 = Slider<tinygpu::RGB565>;
using SliderRGB666 = Slider<tinygpu::RGB666>;
using SliderRGB888 = Slider<tinygpu::RGB888>;

}  // namespace tinymd
