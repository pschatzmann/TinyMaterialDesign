#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Top app bar: a title plus optional leading/trailing icon widgets
 * (typically IconButton).
 *
 * `leading`/`trailing` are children AppBar forwards draw()/update()/
 * onGesture() to internally - do not also add them to Screen directly, or
 * they'd be hit-tested twice. Position them with leadingRect()/
 * trailingRect() before attaching.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class AppBar : public Widget<RGB_T> {
 public:
  AppBar() = default;
  AppBar(Bounds bounds, const char* title) : title_(title) { this->bounds = bounds; }

  Widget<RGB_T>* leading = nullptr;
  Widget<RGB_T>* trailing = nullptr;

  void setTitle(const char* title) { title_ = title; }

  /// Suggested square icon slot at the bar's left edge.
  Bounds leadingRect() const {
    const int32_t size = this->bounds.h - 8;
    return Bounds(this->bounds.x + 4, this->bounds.y + (this->bounds.h - size) / 2, size, size);
  }
  /// Suggested square icon slot at the bar's right edge.
  Bounds trailingRect() const {
    const int32_t size = this->bounds.h - 8;
    return Bounds(this->bounds.right() - size - 4, this->bounds.y + (this->bounds.h - size) / 2,
               size, size);
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), theme.colors.surface);

    tinygpu::IFont<RGB_T>& font = *theme.typography.title;
    const size_t textHeight = font.getHeight(1);
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(textHeight) / 2;
    const int32_t textX = leading != nullptr ? leadingRect().right() + 8 : this->bounds.x + 16;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 title_.c_str(), theme.colors.onSurface, theme.colors.surface, false);

    if (leading != nullptr && leading->visible) leading->draw(target, theme);
    if (trailing != nullptr && trailing->visible) trailing->draw(target, theme);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (leading != nullptr && leading->enabled &&
        leading->bounds.contains(event.point.x, event.point.y)) {
      return leading->onGesture(event);
    }
    if (trailing != nullptr && trailing->enabled &&
        trailing->bounds.contains(event.point.x, event.point.y)) {
      return trailing->onGesture(event);
    }
    return false;
  }

  void update(uint32_t nowMs) override {
    if (leading != nullptr) leading->update(nowMs);
    if (trailing != nullptr) trailing->update(nowMs);
  }

 private:
  std::string title_;
};

using AppBarRGB565 = AppBar<tinygpu::RGB565>;
using AppBarRGB666 = AppBar<tinygpu::RGB666>;
using AppBarRGB888 = AppBar<tinygpu::RGB888>;

}  // namespace tinymd
