#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Small floating label, shown briefly above an anchor widget.
 *
 * Not driven by hover (this library targets touchscreens, not pointers) -
 * call showFor(anchorBounds, text) yourself, typically from a long-press
 * gesture on the widget it explains, and it hides itself automatically
 * after its duration via update(). Add once with Screen::addFixedWidget()
 * (last, so it draws on top); starts invisible so it costs nothing while
 * idle.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Tooltip : public Widget<RGB_T> {
 public:
  Tooltip() { this->visible = false; }

  /// Shows `text` centered just above `anchor` for `durationMs`.
  void showFor(Bounds anchor, const char* text, uint32_t durationMs = 1500) {
    text_ = text != nullptr ? text : "";
    durationMs_ = durationMs;

    tinygpu::IFont<RGB_T>* font = this->theme_ != nullptr ? this->theme().typography.label : nullptr;
    const int32_t textWidth = font != nullptr ? static_cast<int32_t>(font->measureTextWidth(text_.c_str())) : 0;
    const int32_t textHeight = font != nullptr ? static_cast<int32_t>(font->getHeight(1)) : 12;
    const int32_t pad = 8;
    const int32_t w = textWidth + pad * 2;
    const int32_t h = textHeight + pad;
    this->bounds = Bounds(anchor.centerX() - w / 2, anchor.y - h - 6, w, h);

    this->visible = true;
    started_ = false;
  }

  void hide() { this->visible = false; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const RGB_T background = this->theme().colors.onSurface;
    const RGB_T foreground = this->theme().colors.surface;
    const size_t radius = toPx(this->theme().shape.small);

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;
    const size_t textWidth = font.measureTextWidth(text_.c_str());
    const int32_t textX = this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), text_.c_str(),
                 foreground, background, false);
  }

  bool update(uint32_t nowMs) override {
    if (!this->visible) return false;
    if (!started_) {
      started_ = true;
      startMs_ = nowMs;
      return true;
    }
    if (nowMs - startMs_ >= durationMs_) {
      this->visible = false;
      return true;
    }
    return false;
  }

 private:
  std::string text_;
  uint32_t durationMs_ = 1500;
  bool started_ = false;
  uint32_t startMs_ = 0;
};

using TooltipRGB565 = Tooltip<tinygpu::RGB565>;
using TooltipRGB666 = Tooltip<tinygpu::RGB666>;
using TooltipRGB888 = Tooltip<tinygpu::RGB888>;

}  // namespace tinymd
