#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyGPU/Font/LinePrinter.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Modal alert dialog: full-screen scrim + centered card + title +
 * wrapped message + up to 2 action widgets (typically Button).
 *
 * Show it with Screen::presentDialog(dialog); while presented, Screen routes
 * every gesture to the dialog first. Actions (usually Button) are not owned
 * by Dialog - position each one's bounds (actionRect() suggests a spot),
 * add it with addAction(), and wire its onClick to call
 * screen.dismissDialog() itself.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Dialog : public Widget<RGB_T> {
 public:
  Dialog() = default;
  /// `cardBounds` is the dialog card's own rect, not the full screen - the
  /// scrim is drawn separately, covering the whole target surface.
  Dialog(Bounds cardBounds, const char* title, const char* message)
      : title_(title), message_(message) {
    this->bounds = cardBounds;
  }

  void setTitle(const char* title) { title_ = title; }
  void setMessage(const char* message) { message_ = message; }

  void addAction(Widget<RGB_T>& action) {
    if (actionCount_ < kMaxActions) actions_[actionCount_++] = &action;
  }

  /// Suggested rect for the `index`-th of `count` actions, right-aligned
  /// along the card's bottom edge.
  Bounds actionRect(int index, int count, int32_t width = 80, int32_t height = 36) const {
    const int32_t pad = 8;
    const int32_t totalWidth = count * width + (count - 1) * pad;
    const int32_t startX = this->bounds.right() - pad - totalWidth;
    const int32_t y = this->bounds.bottom() - pad - height;
    return Bounds(startX + index * (width + pad), y, width, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    const RGB_T scrim = blend(theme.colors.onBackground, RGB_T(0, 0, 0), 0.35f);
    target.fillRect(0, 0, target.width(), target.height(), scrim);

    const size_t radius = toPx(theme.shape.large);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, theme.colors.surface);
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, theme.colors.outline);

    const int32_t pad = theme.spacing;
    int32_t cursorY = this->bounds.y + pad * 2;

    tinygpu::IFont<RGB_T>& titleFont = *theme.typography.title;
    titleFont.drawText(target, static_cast<int16_t>(this->bounds.x + pad),
                       static_cast<int16_t>(cursorY), title_.c_str(), theme.colors.onSurface,
                       theme.colors.surface, false);
    cursorY += static_cast<int32_t>(titleFont.getHeight(1)) + pad;

    const int32_t rightEdge = this->bounds.right() - pad;
    const size_t rightBorder = rightEdge < static_cast<int32_t>(target.width())
                                  ? target.width() - static_cast<size_t>(rightEdge)
                                  : 0;
    tinygpu::LinePrinter<RGB_T> printer;
    printer.setFont(*theme.typography.body);
    printer.setTarget(target);
    printer.setColor(theme.colors.onSurfaceVariant);
    printer.setTopBorder(toPx(cursorY));
    printer.setLeftBorder(toPx(this->bounds.x + pad));
    printer.setRightBorder(rightBorder);
    printer.print(message_.c_str());

    for (int i = 0; i < actionCount_; ++i) {
      if (actions_[i]->visible) actions_[i]->draw(target, theme);
    }
  }

  /// Screen routes every gesture here while the dialog is presented; only
  /// taps landing on an action are forwarded to it, everything else is
  /// swallowed (a modal dialog shouldn't leak touches through to whatever
  /// is behind it).
  bool onGesture(const tinygpu::GestureEvent& event) override {
    for (int i = 0; i < actionCount_; ++i) {
      if (actions_[i]->enabled && actions_[i]->bounds.contains(event.point.x, event.point.y)) {
        return actions_[i]->onGesture(event);
      }
    }
    return true;
  }

  void update(uint32_t nowMs) override {
    for (int i = 0; i < actionCount_; ++i) actions_[i]->update(nowMs);
  }

 private:
  static constexpr int kMaxActions = 2;

  std::string title_;
  std::string message_;
  Widget<RGB_T>* actions_[kMaxActions] = {nullptr, nullptr};
  int actionCount_ = 0;
};

using DialogRGB565 = Dialog<tinygpu::RGB565>;
using DialogRGB666 = Dialog<tinygpu::RGB666>;
using DialogRGB888 = Dialog<tinygpu::RGB888>;

}  // namespace tinymd
