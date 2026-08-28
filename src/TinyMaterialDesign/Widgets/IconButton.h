#pragma once
#include <algorithm>
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Draw/Icons.h"

namespace tinymd {

/// Signature every Draw/Icons.h glyph function matches: draw `color`, with
/// stroke width `thickness`, into `bounds` on `target`. A function pointer's
/// type reflects every declared parameter regardless of default arguments,
/// so this must list all 4 even though most Icons.h functions default the
/// last one.
template <typename RGB_T>
using IconPainter = void (*)(tinygpu::ISurface<RGB_T>&, const Bounds&, RGB_T, uint8_t);

/// kStandard is a plain flat tap target (the original IconButton look - app
/// bar/toolbar icons). kFilled/kTonal add a colored, elevated circular
/// background, i.e. a Material "Floating Action Button" - set a large
/// enough bounds (e.g. 56x56) and pair it with setColorOverride() for a
/// status-dependent FAB (a play/stop control that's green or red, say).
enum class IconButtonVariant { kStandard, kFilled, kTonal };

/// Circular tap target hosting one vector glyph from Draw/Icons.h (or any
/// function matching IconPainter). Tap feedback is a ripple, same rationale
/// as Button (see Button.h's class comment).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class IconButton : public Widget<RGB_T> {
 public:
  IconButton() = default;
  IconButton(Bounds bounds, IconPainter<RGB_T> icon,
            IconButtonVariant variant = IconButtonVariant::kStandard)
      : icon_(icon), variant_(variant) {
    this->bounds = bounds;
  }

  std::function<void()> onClick;

  void setIcon(IconPainter<RGB_T> icon) { icon_ = icon; }
  void setVariant(IconButtonVariant variant) { variant_ = variant; }

  /// Overrides the variant's theme colors - e.g. a FAB that's green when
  /// idle and red while active. Call clearColorOverride() to go back to
  /// the variant's normal theme colors.
  void setColorOverride(RGB_T background, RGB_T foreground) {
    hasColorOverride_ = true;
    backgroundOverride_ = background;
    foregroundOverride_ = foreground;
  }
  void clearColorOverride() { hasColorOverride_ = false; }

  /// See Widget::setThemeTint() - used only when this button has no
  /// setColorOverride() of its own.
  void setThemeTint(RGB_T background, RGB_T foreground) override {
    hasAmbientTint_ = true;
    ambientBackground_ = background;
    ambientForeground_ = foreground;
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    RGB_T background = theme.colors.surface;
    RGB_T foreground = theme.colors.onSurfaceVariant;
    const bool paintBackground = variant_ != IconButtonVariant::kStandard;

    switch (variant_) {
      case IconButtonVariant::kFilled:
        background = theme.colors.primary;
        foreground = theme.colors.onPrimary;
        break;
      case IconButtonVariant::kTonal:
        background = theme.colors.secondaryContainer;
        foreground = theme.colors.onSecondaryContainer;
        break;
      case IconButtonVariant::kStandard:
      default:
        break;
    }
    // Priority: explicit override > container's ambient tint > variant
    // default - see setColorOverride()/setThemeTint().
    if (hasAmbientTint_) {
      background = ambientBackground_;
      foreground = ambientForeground_;
    }
    if (hasColorOverride_) {
      background = backgroundOverride_;
      foreground = foregroundOverride_;
    }
    if (!this->enabled) {
      background = blend(background, theme.colors.surface, 0.6f);
      foreground = blend(foreground, theme.colors.surface, 0.5f);
    }

    const int32_t diameter = this->bounds.w < this->bounds.h ? this->bounds.w : this->bounds.h;
    const size_t radius = toPx(diameter / 2);

    if (paintBackground) {
      if (this->enabled) {
        const RGB_T shadow = blend(theme.colors.surface, theme.colors.onBackground, 0.35f);
        target.fillCircle(toPx(this->bounds.centerX() + 1), toPx(this->bounds.centerY() + 2),
                          radius, shadow);
      }
      target.fillCircle(toPx(this->bounds.centerX()), toPx(this->bounds.centerY()), radius,
                        background);
    }

    if (theme.enableRipple && rippleActive_) {
      const uint32_t elapsed = lastUpdateMs_ - rippleStartMs_;
      const float t = std::min(1.0f, static_cast<float>(elapsed) / kRippleDurationMs);
      const size_t rippleRadius = toPx(static_cast<int32_t>(t * diameter / 2));
      const RGB_T rippleColor = blend(background, foreground, 0.15f * (1.0f - t));
      target.fillCircle(toPx(this->bounds.centerX()), toPx(this->bounds.centerY()), rippleRadius,
                        rippleColor);
    }

    if (icon_ != nullptr) {
      const Bounds iconRect = this->bounds.inset(this->bounds.w / 4);
      icon_(target, iconRect, foreground, /*thickness=*/2);
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
    if (rippleActive_ && (nowMs - rippleStartMs_) >= kRippleDurationMs) {
      rippleActive_ = false;
    }
    return wasActive;
  }

 private:
  static constexpr uint32_t kRippleDurationMs = 220;

  IconPainter<RGB_T> icon_ = nullptr;
  IconButtonVariant variant_ = IconButtonVariant::kStandard;
  bool hasColorOverride_ = false;
  RGB_T backgroundOverride_{};
  RGB_T foregroundOverride_{};
  bool hasAmbientTint_ = false;
  RGB_T ambientBackground_{};
  RGB_T ambientForeground_{};
  bool rippleActive_ = false;
  uint32_t rippleStartMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
};

using IconButtonRGB565 = IconButton<tinygpu::RGB565>;
using IconButtonRGB666 = IconButton<tinygpu::RGB666>;
using IconButtonRGB888 = IconButton<tinygpu::RGB888>;

}  // namespace tinymd
