#pragma once
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// @brief Track-and-thumb toggle switch. Toggled by tap (drag-to-slide is a
/// possible follow-up, not implemented here).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Switch : public Widget<RGB_T> {
 public:
  Switch() = default;
  explicit Switch(Bounds bounds, bool value = false) : value_(value) { this->bounds = bounds; }

  std::function<void(bool)> onChange;

  bool value() const { return value_; }
  void setValue(bool value) { value_ = value; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T trackColor = value_ ? this->theme().colors.primary : this->theme().colors.surfaceVariant;
    RGB_T trackBorder = this->theme().colors.outline;
    RGB_T thumbColor = value_ ? this->theme().colors.onPrimary : this->theme().colors.outline;
    if (!this->enabled) {
      trackColor = blend(trackColor, this->theme().colors.surface, 0.5f);
      trackBorder = blend(trackBorder, this->theme().colors.surface, 0.5f);
      thumbColor = blend(thumbColor, this->theme().colors.surface, 0.4f);
    }

    const size_t trackRadius = toPx(this->bounds.h / 2);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                         toPx(this->bounds.w), toPx(this->bounds.h), trackRadius,
                         trackColor);
    if (!value_) {
      target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                           toPx(this->bounds.w), toPx(this->bounds.h), trackRadius,
                           trackBorder);
    }

    const int32_t thumbRadius = this->bounds.h / 2 - 3;
    const int32_t thumbCx =
        value_ ? this->bounds.right() - this->bounds.h / 2 : this->bounds.x + this->bounds.h / 2;
    const int32_t thumbCy = this->bounds.centerY();
    target.fillCircle(toPx(thumbCx), toPx(thumbCy), toPx(thumbRadius), thumbColor);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    value_ = !value_;
    if (onChange) onChange(value_);
    return true;
  }

 private:
  bool value_ = false;
};

using SwitchRGB565 = Switch<tinygpu::RGB565>;
using SwitchRGB666 = Switch<tinygpu::RGB666>;
using SwitchRGB888 = Switch<tinygpu::RGB888>;

}  // namespace tinymd
