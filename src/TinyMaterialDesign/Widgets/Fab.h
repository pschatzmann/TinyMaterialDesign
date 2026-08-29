#pragma once
#include <algorithm>
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Bounds.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"  // IconPainter

namespace tinymd {

/// kSurface (default M3 look) / kPrimary / kSecondary / kTertiary pick which
/// theme color pair the FAB is filled with.
enum class FabColor { kSurface, kPrimary, kSecondary, kTertiary };

/**
 * @brief Floating Action Button: a circular (or pill, when it has a label)
 * elevated icon button for a screen's single primary action.
 *
 * Construct with just an icon for the standard circular FAB, or with a
 * label too for the "extended" pill-shaped variant. Bounds picks the size -
 * conventionally 40x40 (small), 56x56 (regular), 96x96 (large) for the
 * circular form, or width x 56 for extended.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class FloatingActionButton : public Widget<RGB_T> {
 public:
  FloatingActionButton() = default;
  FloatingActionButton(Bounds bounds, IconPainter<RGB_T> icon, const char* label = nullptr,
      FabColor color = FabColor::kPrimary)
      : icon_(icon), color_(color) {
    this->bounds = bounds;
    if (label != nullptr) {
      label_ = label;
      extended_ = true;
    }
  }

  std::function<void()> onClick;

  void setIcon(IconPainter<RGB_T> icon) { icon_ = icon; }
  void setLabel(const char* label) {
    label_ = label != nullptr ? label : "";
    extended_ = !label_.empty();
  }
  void setColor(FabColor color) { color_ = color; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T background;
    RGB_T foreground;
    switch (color_) {
      case FabColor::kPrimary:
        background = this->theme().colors.primaryContainer;
        foreground = this->theme().colors.onPrimaryContainer;
        break;
      case FabColor::kSecondary:
        background = this->theme().colors.secondaryContainer;
        foreground = this->theme().colors.onSecondaryContainer;
        break;
      case FabColor::kTertiary:
        background = this->theme().colors.secondary;
        foreground = this->theme().colors.onSecondary;
        break;
      case FabColor::kSurface:
      default:
        background = this->theme().colors.surfaceVariant;
        foreground = this->theme().colors.primary;
        break;
    }
    if (!this->enabled) {
      background = blend(background, this->theme().colors.surface, 0.6f);
      foreground = blend(foreground, this->theme().colors.surface, 0.5f);
    }

    const size_t radius = extended_ ? toPx(this->theme().shape.large) : toPx(this->bounds.h / 2);

    if (this->enabled) {
      const RGB_T shadow = blend(this->theme().colors.surface, this->theme().colors.onBackground, 0.4f);
      target.fillRoundRect(toPx(this->bounds.x + 1), toPx(this->bounds.y + 2), toPx(this->bounds.w),
                           toPx(this->bounds.h), radius, shadow);
    }
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);

    if (this->theme().enableRipple && rippleActive_) {
      const uint32_t elapsed = lastUpdateMs_ - rippleStartMs_;
      const float t = std::min(1.0f, static_cast<float>(elapsed) / kRippleDurationMs);
      const int32_t maxRadius = this->bounds.w > this->bounds.h ? this->bounds.w : this->bounds.h;
      const RGB_T rippleColor = blend(background, foreground, 0.18f * (1.0f - t));
      target.fillCircle(toPx(this->bounds.centerX()), toPx(this->bounds.centerY()),
                        toPx(static_cast<int32_t>(t * maxRadius)), rippleColor);
    }

    const int32_t iconSize = std::min<int32_t>(this->bounds.h - 16, 32);
    const int32_t iconPad = extended_ ? this->theme().spacing * 2 : (this->bounds.w - iconSize) / 2;
    const Bounds iconRect(this->bounds.x + iconPad, this->bounds.centerY() - iconSize / 2, iconSize,
                          iconSize);
    if (icon_ != nullptr) icon_(target, iconRect, foreground, /*thickness=*/2);

    if (extended_) {
      tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;
      const int32_t textX = icon_ != nullptr ? iconRect.right() + this->theme().spacing
                                             : this->bounds.x + this->theme().spacing * 2;
      const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), label_.c_str(),
                   foreground, background, false);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    rippleStartMs_ = lastUpdateMs_;
    rippleActive_ = true;
    if (onClick) onClick();
    return true;
  }

  bool update(uint32_t nowMs) override {
    const bool wasActive = rippleActive_;
    lastUpdateMs_ = nowMs;
    if (rippleActive_ && (nowMs - rippleStartMs_) >= kRippleDurationMs) rippleActive_ = false;
    return wasActive;
  }

 private:
  static constexpr uint32_t kRippleDurationMs = 260;

  IconPainter<RGB_T> icon_ = nullptr;
  std::string label_;
  bool extended_ = false;
  FabColor color_ = FabColor::kPrimary;

  bool rippleActive_ = false;
  uint32_t rippleStartMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
};

using FAB = FloatingActionButton<TINYMD_DEFAULT_RGB_T>;

using FloatingActionButtonRGB565 = FloatingActionButton<tinygpu::RGB565>;
using FloatingActionButtonRGB666 = FloatingActionButton<tinygpu::RGB666>;
using FloatingActionButtonRGB888 = FloatingActionButton<tinygpu::RGB888>;
using FabRGB565 = FloatingActionButtonRGB565;
using FabRGB666 = FloatingActionButtonRGB666;
using FabRGB888 = FloatingActionButtonRGB888;

}  // namespace tinymd
