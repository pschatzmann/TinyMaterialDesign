#pragma once
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Draw/Icons.h"

namespace tinymd {

/// Square checkbox, toggled by tap.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Checkbox : public Widget<RGB_T> {
 public:
  Checkbox() = default;
  explicit Checkbox(Bounds bounds, bool checked = false) : checked_(checked) {
    this->bounds = bounds;
  }

  std::function<void(bool)> onChange;

  bool checked() const { return checked_; }
  void setChecked(bool checked) { checked_ = checked; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T fill = checked_ ? this->theme().colors.primary : this->theme().colors.surface;
    RGB_T border = checked_ ? this->theme().colors.primary : this->theme().colors.outline;
    RGB_T mark = this->theme().colors.onPrimary;
    if (!this->enabled) {
      fill = blend(fill, this->theme().colors.surface, 0.6f);
      border = blend(border, this->theme().colors.surface, 0.5f);
      mark = blend(mark, this->theme().colors.surface, 0.4f);
    }

    const size_t radius = toPx(this->theme().shape.small);
    if (checked_) {
      target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                           toPx(this->bounds.w), toPx(this->bounds.h), radius, fill);
    }
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                         toPx(this->bounds.w), toPx(this->bounds.h), radius, border);
    if (checked_) {
      drawCheckmark(target, this->bounds, mark);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    checked_ = !checked_;
    if (onChange) onChange(checked_);
    return true;
  }

 private:
  bool checked_ = false;
};

using CheckboxRGB565 = Checkbox<tinygpu::RGB565>;
using CheckboxRGB666 = Checkbox<tinygpu::RGB666>;
using CheckboxRGB888 = Checkbox<tinygpu::RGB888>;

}  // namespace tinymd
