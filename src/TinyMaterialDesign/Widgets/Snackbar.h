#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Transient bottom message bar with an optional single text action.
 *
 * Not a modal - add it once with Screen::addFixedWidget() (last, so it
 * draws on top) and drive its lifetime with show()/dismiss(); it hides
 * itself automatically after its duration via update(), same mechanism a
 * ripple fade uses. visible stays false until show() is called, so it costs
 * nothing extra when idle - Screen already skips invisible widgets.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Snackbar : public Widget<RGB_T> {
 public:
  Snackbar() { this->visible = false; }
  explicit Snackbar(Bounds bounds) {
    this->bounds = bounds;
    this->visible = false;
  }

  /// Fired when the (optional) action text is tapped.
  std::function<void()> onAction;
  /// Fired when the snackbar hides itself, whether by timeout or dismiss().
  std::function<void()> onDismiss;

  /// Shows the bar with `message` (and optional `actionLabel`) for
  /// `durationMs` (default 4s, matching M3 guidance).
  void show(const char* message, const char* actionLabel = nullptr, uint32_t durationMs = 4000) {
    message_ = message != nullptr ? message : "";
    actionLabel_ = actionLabel != nullptr ? actionLabel : "";
    durationMs_ = durationMs;
    this->visible = true;
    started_ = false;
  }

  void dismiss() {
    if (!this->visible) return;
    this->visible = false;
    if (onDismiss) onDismiss();
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const RGB_T background = this->theme().colors.onSurface;
    const RGB_T foreground = this->theme().colors.surface;
    const size_t radius = toPx(this->theme().shape.small);

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.body;
    const int32_t pad = this->theme().spacing * 2;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
    font.drawText(target, static_cast<int16_t>(this->bounds.x + pad), static_cast<int16_t>(textY),
                 message_.c_str(), foreground, background, false);

    if (!actionLabel_.empty()) {
      tinygpu::IFont<RGB_T>& actionFont = *this->theme().typography.label;
      const size_t actionWidth = actionFont.measureTextWidth(actionLabel_.c_str());
      const int32_t actionX = this->bounds.right() - pad - static_cast<int32_t>(actionWidth);
      const int32_t actionY = this->bounds.centerY() - static_cast<int32_t>(actionFont.getHeight(1)) / 2;
      actionFont.drawText(target, static_cast<int16_t>(actionX), static_cast<int16_t>(actionY),
                         actionLabel_.c_str(), this->theme().colors.primaryContainer, background, false);
      actionRect_ = Bounds(actionX - 8, this->bounds.y, static_cast<int32_t>(actionWidth) + 16,
                           this->bounds.h);
    } else {
      actionRect_ = Bounds();
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (!actionLabel_.empty() && actionRect_.contains(event.point.x, event.point.y)) {
      if (onAction) onAction();
    }
    dismiss();
    return true;
  }

  bool update(uint32_t nowMs) override {
    if (!this->visible) return false;
    if (!started_) {
      started_ = true;
      startMs_ = nowMs;
      return true;  // just became visible
    }
    if (nowMs - startMs_ >= durationMs_) {
      dismiss();
      return true;
    }
    return false;
  }

 private:
  std::string message_;
  std::string actionLabel_;
  Bounds actionRect_;
  uint32_t durationMs_ = 4000;
  bool started_ = false;
  uint32_t startMs_ = 0;
};

using SnackbarRGB565 = Snackbar<tinygpu::RGB565>;
using SnackbarRGB666 = Snackbar<tinygpu::RGB666>;
using SnackbarRGB888 = Snackbar<tinygpu::RGB888>;

}  // namespace tinymd
