#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyGPU/Font/LinePrinter.h"

namespace tinymd {

/**
 * @brief Persistent, non-modal inline message with up to 2 text actions -
 * Material's "banner", e.g. "You're offline. [Retry] [Dismiss]" pinned
 * below an app bar.
 *
 * Unlike Dialog/Snackbar this isn't presented modally - add it with
 * Screen::addWidget()/addFixedWidget() like any other widget and toggle
 * `visible` yourself. Actions are not owned by Banner - same non-owning
 * addAction() convention as Dialog::addAction().
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Banner : public Widget<RGB_T> {
 public:
  Banner() = default;
  Banner(Bounds bounds, const char* message) : message_(message != nullptr ? message : "") {
    this->bounds = bounds;
  }

  void setMessage(const char* message) { message_ = message != nullptr ? message : ""; }

  void addAction(Widget<RGB_T>& action) {
    if (actionCount_ < kMaxActions) {
      actions_[actionCount_++] = &action;
      if (this->theme_ != nullptr) action.setTheme(*this->theme_);
    }
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (int i = 0; i < actionCount_; ++i) {
      if (actions_[i] != nullptr) actions_[i]->setTheme(theme);
    }
  }

  /// Suggested rect for the `index`-th of `count` actions, right-aligned
  /// along the banner's bottom edge - same layout convention as
  /// Dialog::actionRect().
  Bounds actionRect(int index, int count, int32_t width = 80, int32_t height = 32) const {
    const int32_t pad = 8;
    const int32_t totalWidth = count * width + (count - 1) * pad;
    const int32_t startX = this->bounds.right() - pad - totalWidth;
    const int32_t y = this->bounds.bottom() - pad - height;
    return Bounds(startX + index * (width + pad), y, width, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), this->theme().colors.surfaceVariant);
    target.drawLine(toPx(this->bounds.x), toPx(this->bounds.bottom() - 1), toPx(this->bounds.right()),
                    toPx(this->bounds.bottom() - 1), this->theme().colors.outline);

    const int32_t pad = this->theme().spacing;
    const int32_t rightEdge = this->bounds.right() - pad;
    const size_t rightBorder = rightEdge < static_cast<int32_t>(target.width())
                                  ? target.width() - static_cast<size_t>(rightEdge)
                                  : 0;
    tinygpu::LinePrinter<RGB_T> printer;
    printer.setFont(*this->theme().typography.body);
    printer.setTarget(target);
    printer.setColor(this->theme().colors.onSurfaceVariant);
    printer.setTopBorder(toPx(this->bounds.y + pad));
    printer.setLeftBorder(toPx(this->bounds.x + pad));
    printer.setRightBorder(rightBorder);
    printer.print(message_.c_str());

    for (int i = 0; i < actionCount_; ++i) {
      if (actions_[i]->visible) actions_[i]->draw(target);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    for (int i = 0; i < actionCount_; ++i) {
      if (actions_[i]->enabled && actions_[i]->bounds.contains(event.point.x, event.point.y)) {
        return actions_[i]->onGesture(event);
      }
    }
    return false;
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    for (int i = 0; i < actionCount_; ++i) changed |= actions_[i]->update(nowMs);
    return changed;
  }

 private:
  static constexpr int kMaxActions = 2;

  std::string message_;
  Widget<RGB_T>* actions_[kMaxActions] = {nullptr, nullptr};
  int actionCount_ = 0;
};

using BannerRGB565 = Banner<tinygpu::RGB565>;
using BannerRGB666 = Banner<tinygpu::RGB666>;
using BannerRGB888 = Banner<tinygpu::RGB888>;

}  // namespace tinymd
