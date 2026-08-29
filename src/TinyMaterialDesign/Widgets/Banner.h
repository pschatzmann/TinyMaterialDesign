#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyGPU/Font/LinePrinter.h"

namespace tinymd {

/**
 * @brief Persistent, non-modal inline message with text actions - Material's
 * "banner", e.g. "You're offline. [Retry] [Dismiss]" pinned below an app
 * bar.
 *
 * Unlike Dialog/Snackbar this isn't presented modally - add it with
 * Screen::addWidget()/addFixedWidget() like any other widget and toggle
 * `visible` yourself. Actions are not owned by Banner - same non-owning
 * addAction() convention as Dialog::addAction().
 *
 * Alternative to addAction(): setActionProvider(count, at) switches to
 * callback-driven actions - see Container.h's class comment for the pattern.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Banner : public Widget<RGB_T> {
 public:
  using ActionCountFn = std::function<int()>;
  using ActionAtFn = std::function<Widget<RGB_T>&(int index)>;

  Banner() = default;
  Banner(Bounds bounds, const char* message) : message_(message != nullptr ? message : "") {
    this->bounds = bounds;
  }

  void setMessage(const char* message) { message_ = message != nullptr ? message : ""; }

  /// Registers `action` as content of this banner. Ignored while an action
  /// provider is active - see setActionProvider().
  void addAction(Widget<RGB_T>& action) {
    actions_.push_back(&action);
    if (this->theme_ != nullptr) action.setTheme(*this->theme_);
  }

  /// Switches to callback-driven actions - see the class comment.
  void setActionProvider(ActionCountFn count, ActionAtFn at) {
    actionCountFn_ = std::move(count);
    actionAtFn_ = std::move(at);
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (Widget<RGB_T>* action : actions_) {
      if (action != nullptr) action->setTheme(theme);
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
    // Crops a message too long for the banner to its own bounds instead of
    // letting it spill past the bottom edge (see ISurface::pushClipRect()).
    target.pushClipRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                        toPx(this->bounds.h));
    tinygpu::LinePrinter<RGB_T> printer;
    printer.setFont(*this->theme().typography.body);
    printer.setTarget(target);
    printer.setColor(this->theme().colors.onSurfaceVariant);
    printer.setTopBorder(toPx(this->bounds.y + pad));
    printer.setLeftBorder(toPx(this->bounds.x + pad));
    printer.setRightBorder(rightBorder);
    printer.print(message_.c_str());
    target.popClipRect();

    const int count = actionCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* action = actionAt(i);
      if (action->visible) action->draw(target);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    const int count = actionCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* action = actionAt(i);
      if (action->enabled && action->bounds.contains(event.point.x, event.point.y)) {
        return action->onGesture(event);
      }
    }
    return false;
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    const int count = actionCount();
    for (int i = 0; i < count; ++i) changed |= actionAt(i)->update(nowMs);
    return changed;
  }

  /// True if there's an action at (x, y) that's draggable, recursing into
  /// it if it's itself a composite - see Widget::isDraggableAt().
  bool isDraggableAt(int32_t x, int32_t y) const override {
    const int count = actionCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* action = actionAt(i);
      if (action->enabled && action->bounds.contains(x, y)) return action->isDraggableAt(x, y);
    }
    return false;
  }

 private:
  int actionCount() const {
    return actionCountFn_ ? actionCountFn_() : static_cast<int>(actions_.size());
  }

  Widget<RGB_T>* actionAt(int index) const {
    if (actionAtFn_) {
      Widget<RGB_T>& action = actionAtFn_(index);
      if (this->theme_ != nullptr) action.setTheme(*this->theme_);
      return &action;
    }
    return actions_[static_cast<size_t>(index)];
  }

  std::string message_;
  std::vector<Widget<RGB_T>*> actions_;
  ActionCountFn actionCountFn_;
  ActionAtFn actionAtFn_;
};

using BannerRGB565 = Banner<tinygpu::RGB565>;
using BannerRGB666 = Banner<tinygpu::RGB666>;
using BannerRGB888 = Banner<tinygpu::RGB888>;

}  // namespace tinymd
