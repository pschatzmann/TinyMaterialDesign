#pragma once
#include <algorithm>
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Bounds.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

enum class ButtonVariant { kFilled, kTonal, kOutlined, kText, kElevated };

/**
 * @brief Material Design button: Filled / Tonal / Outlined / Text / Elevated.
 *
 * Tap feedback is the (optional) ripple grown from the tap point - real
 * touch drivers only report a recognized, completed tap (GestureType::kTap),
 * not a raw finger-down instant, so the ripple *is* the button's touch
 * feedback rather than a separate immediate "pressed" shade.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Button : public Widget<RGB_T> {
 public:
  Button() = default;
  Button(Bounds bounds, const char* label, ButtonVariant variant = ButtonVariant::kFilled)
      : label_(label), variant_(variant) {
    this->bounds = bounds;
  }

  std::function<void()> onClick;

  void setLabel(const char* label) { label_ = label; }
  const std::string& label() const { return label_; }
  void setVariant(ButtonVariant variant) { variant_ = variant; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    RGB_T background;
    RGB_T foreground;
    bool paintBackground = true;
    bool paintOutline = false;

    switch (variant_) {
      case ButtonVariant::kFilled:
        background = theme.colors.primary;
        foreground = theme.colors.onPrimary;
        break;
      case ButtonVariant::kTonal:
        background = theme.colors.secondaryContainer;
        foreground = theme.colors.onSecondaryContainer;
        break;
      case ButtonVariant::kElevated:
        background = theme.colors.surface;
        foreground = theme.colors.primary;
        break;
      case ButtonVariant::kOutlined:
        background = theme.colors.surface;
        foreground = theme.colors.primary;
        paintOutline = true;
        break;
      case ButtonVariant::kText:
      default:
        background = theme.colors.surface;
        foreground = theme.colors.primary;
        paintBackground = false;
        break;
    }

    if (!this->enabled) {
      foreground = blend(foreground, theme.colors.onSurface, 0.5f);
      background = blend(background, theme.colors.onSurface, 0.7f);
    }

    const int32_t radius = theme.shape.full;  // pill-shaped, standard M3

    if (variant_ == ButtonVariant::kElevated && this->enabled) {
      const RGB_T shadow = blend(theme.colors.surface, theme.colors.onBackground, 0.35f);
      target.fillRoundRect(toPx(this->bounds.x + 1), toPx(this->bounds.y + 2),
                           toPx(this->bounds.w), toPx(this->bounds.h), toPx(radius),
                           shadow);
    }

    if (paintBackground) {
      target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                           toPx(this->bounds.w), toPx(this->bounds.h), toPx(radius),
                           background);
    }
    if (paintOutline) {
      const RGB_T outlineColor =
          this->enabled ? theme.colors.outline : blend(theme.colors.outline, theme.colors.surface, 0.5f);
      target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y),
                           toPx(this->bounds.w), toPx(this->bounds.h), toPx(radius),
                           outlineColor);
    }

    if (theme.enableRipple && rippleActive_) {
      drawRipple(target, background, foreground);
    }

    tinygpu::IFont<RGB_T>& font = *theme.typography.label;
    const size_t textWidth = font.measureTextWidth(label_.c_str());
    const size_t textHeight = font.getHeight(1);
    const int32_t textX = this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(textHeight) / 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 label_.c_str(), foreground, background, false);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    rippleOriginX_ = event.point.x;
    rippleOriginY_ = event.point.y;
    // lastUpdateMs_ is one frame stale (update() hasn't run yet this frame),
    // which is imperceptible next to a ~260ms ripple.
    rippleStartMs_ = lastUpdateMs_;
    rippleActive_ = true;
    if (onClick) onClick();
    return true;
  }

  void update(uint32_t nowMs) override {
    lastUpdateMs_ = nowMs;
    if (rippleActive_ && (nowMs - rippleStartMs_) >= kRippleDurationMs) {
      rippleActive_ = false;
    }
  }

 private:
  static constexpr uint32_t kRippleDurationMs = 260;

  std::string label_;
  ButtonVariant variant_ = ButtonVariant::kFilled;

  bool rippleActive_ = false;
  uint32_t rippleStartMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
  int32_t rippleOriginX_ = 0;
  int32_t rippleOriginY_ = 0;

  /// Fills a circle growing from the tap point and fading out, clipped to
  /// this button's rectangular bounds (a reasonable approximation - it
  /// doesn't follow the pill's rounded corners, which isn't visible at
  /// typical button sizes since the ripple fades out well before reaching
  /// them).
  void drawRipple(tinygpu::ISurface<RGB_T>& target, RGB_T background, RGB_T foreground) {
    const uint32_t elapsed = lastUpdateMs_ - rippleStartMs_;
    const float t = std::min(1.0f, static_cast<float>(elapsed) / kRippleDurationMs);
    const float maxRadius =
        static_cast<float>(this->bounds.w > this->bounds.h ? this->bounds.w : this->bounds.h);
    const float radius = t * maxRadius;
    const RGB_T rippleColor = blend(background, foreground, 0.20f * (1.0f - t));

    const int32_t left = std::max(this->bounds.x, static_cast<int32_t>(rippleOriginX_ - radius));
    const int32_t right = std::min(this->bounds.right(), static_cast<int32_t>(rippleOriginX_ + radius) + 1);
    const int32_t top = std::max(this->bounds.y, static_cast<int32_t>(rippleOriginY_ - radius));
    const int32_t bottom = std::min(this->bounds.bottom(), static_cast<int32_t>(rippleOriginY_ + radius) + 1);
    const float radiusSq = radius * radius;

    for (int32_t y = top; y < bottom; ++y) {
      for (int32_t x = left; x < right; ++x) {
        const float dx = static_cast<float>(x - rippleOriginX_);
        const float dy = static_cast<float>(y - rippleOriginY_);
        if (dx * dx + dy * dy <= radiusSq) {
          target.setPixel(toPx(x), toPx(y), rippleColor);
        }
      }
    }
  }
};

using ButtonRGB565 = Button<tinygpu::RGB565>;
using ButtonRGB666 = Button<tinygpu::RGB666>;
using ButtonRGB888 = Button<tinygpu::RGB888>;

}  // namespace tinymd
