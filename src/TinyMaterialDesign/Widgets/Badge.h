#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Small status marker, typically overlaid on an icon's corner.
 *
 * Non-interactive. Either a plain dot (no text set) or a small filled pill
 * holding a short count/label (e.g. "3", "9+"). Position its own `bounds`
 * over whatever it's badging - e.g. the top-right corner of an IconButton's
 * bounds - the caller is responsible for that placement, same as any other
 * widget.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Badge : public Widget<RGB_T> {
 public:
  Badge() = default;
  explicit Badge(Bounds bounds, const char* text = nullptr) {
    this->bounds = bounds;
    if (text != nullptr) text_ = text;
  }

  void setText(const char* text) { text_ = text != nullptr ? text : ""; }
  const std::string& text() const { return text_; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T background = this->theme().colors.error;
    RGB_T foreground = this->theme().colors.onError;
    if (!this->enabled) {
      background = blend(background, this->theme().colors.surface, 0.5f);
      foreground = blend(foreground, this->theme().colors.surface, 0.5f);
    }

    if (text_.empty()) {
      const int32_t diameter = this->bounds.w < this->bounds.h ? this->bounds.w : this->bounds.h;
      target.fillCircle(toPx(this->bounds.centerX()), toPx(this->bounds.centerY()),
                        toPx(diameter / 2), background);
      return;
    }

    const size_t radius = toPx(this->bounds.h / 2);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;
    const size_t textWidth = font.measureTextWidth(text_.c_str());
    const int32_t textX = this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), text_.c_str(),
                 foreground, background, false);
  }

 private:
  std::string text_;
};

using BadgeRGB565 = Badge<tinygpu::RGB565>;
using BadgeRGB666 = Badge<tinygpu::RGB666>;
using BadgeRGB888 = Badge<tinygpu::RGB888>;

}  // namespace tinymd
