#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// Small rounded-rect label, optionally a toggle (filter chip) rather than
/// a one-shot action (assist chip).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Chip : public Widget<RGB_T> {
 public:
  Chip() = default;
  Chip(Bounds bounds, const char* label, bool selectable = false, bool selected = false)
      : label_(label), selectable_(selectable), selected_(selected) {
    this->bounds = bounds;
  }

  /// Fires on every tap.
  std::function<void()> onClick;
  /// Fires only when selectable == true, with the new selected state.
  std::function<void(bool)> onChange;

  bool selected() const { return selected_; }
  void setSelected(bool selected) { selected_ = selected; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T background = selected_ ? this->theme().colors.secondaryContainer : this->theme().colors.surfaceVariant;
    RGB_T foreground = selected_ ? this->theme().colors.onSecondaryContainer : this->theme().colors.onSurfaceVariant;
    RGB_T outline = this->theme().colors.outline;
    if (!this->enabled) {
      background = blend(background, this->theme().colors.surface, 0.5f);
      foreground = blend(foreground, this->theme().colors.surface, 0.5f);
      outline = blend(outline, this->theme().colors.surface, 0.5f);
    }

    const size_t radius = toPx(this->bounds.h / 2);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);
    if (!selected_) {
      target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                           toPx(this->bounds.h), radius, outline);
    }

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;
    const size_t textWidth = font.measureTextWidth(label_.c_str());
    const size_t textHeight = font.getHeight(1);
    const int32_t textX = this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(textHeight) / 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 label_.c_str(), foreground, background, false);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (selectable_) {
      selected_ = !selected_;
      if (onChange) onChange(selected_);
    }
    if (onClick) onClick();
    return true;
  }

 private:
  std::string label_;
  bool selectable_ = false;
  bool selected_ = false;
};

using ChipRGB565 = Chip<tinygpu::RGB565>;
using ChipRGB666 = Chip<tinygpu::RGB666>;
using ChipRGB888 = Chip<tinygpu::RGB888>;

}  // namespace tinymd
