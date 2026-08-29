#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyGPU/Font/LinePrinter.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Modal alert dialog: full-screen scrim + centered card + title +
 * wrapped message + action widgets (typically Button).
 *
 * Show it with Screen::presentDialog(dialog); while presented, Screen routes
 * every gesture to the dialog first. Actions (usually Button) are not owned
 * by Dialog - position each one's bounds (actionRect() suggests a spot),
 * add it with addAction(), and wire its onClick to call
 * screen.dismissDialog() itself.
 *
 * Alternative to addAction(): setActionProvider(count, at) switches to
 * callback-driven actions instead - see Container.h's class comment for the
 * same pattern (`at(index)` returns a Widget reference, typically from a
 * small reused pool). Not usually needed for a dialog's handful of actions,
 * but available for consistency.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Dialog : public Widget<RGB_T> {
 public:
  using ActionCountFn = std::function<int()>;
  using ActionAtFn = std::function<Widget<RGB_T>&(int index)>;

  Dialog() = default;
  /// `cardBounds` is the dialog card's own rect, not the full screen - the
  /// scrim is drawn separately, covering the whole target surface.
  Dialog(Bounds cardBounds, const char* title, const char* message)
      : title_(title), message_(message) {
    this->bounds = cardBounds;
  }

  void setTitle(const char* title) { title_ = title; }
  void setMessage(const char* message) { message_ = message; }

  /// Registers `action` as content of this dialog. Ignored while an action
  /// provider is active - see setActionProvider().
  void addAction(Widget<RGB_T>& action) {
    actions_.push_back(&action);
    // Dialog might not have a theme yet if addAction() is called before
    // the dialog itself is registered with Screen (via presentDialog()) -
    // in that case setTheme() below will cascade to this action once it
    // does.
    if (this->theme_ != nullptr) action.setTheme(*this->theme_);
  }

  /// Switches to callback-driven actions - see the class comment.
  void setActionProvider(ActionCountFn count, ActionAtFn at) {
    actionCountFn_ = std::move(count);
    actionAtFn_ = std::move(at);
  }

  /// Cascades the theme down to every action already added - see addAction().
  /// In provider mode, each action is themed at the point it's fetched
  /// instead (see actionAt()).
  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (Widget<RGB_T>* action : actions_) {
      if (action != nullptr) action->setTheme(theme);
    }
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

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    drawScrim(target);

    const size_t radius = toPx(this->theme().shape.large);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.surface);
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.outline);

    const int32_t pad = this->theme().spacing;
    int32_t cursorY = this->bounds.y + pad * 2;

    tinygpu::IFont<RGB_T>& titleFont = *this->theme().typography.title;
    titleFont.drawText(target, static_cast<int16_t>(this->bounds.x + pad),
                       static_cast<int16_t>(cursorY), title_.c_str(), this->theme().colors.onSurface,
                       this->theme().colors.surface, false);
    cursorY += static_cast<int32_t>(titleFont.getHeight(1)) + pad;

    const int32_t rightEdge = this->bounds.right() - pad;
    const size_t rightBorder = rightEdge < static_cast<int32_t>(target.width())
                                  ? target.width() - static_cast<size_t>(rightEdge)
                                  : 0;
    // Crops a message too long for the card to the card's own bounds
    // instead of letting it spill past the bottom edge (see
    // ISurface::pushClipRect()).
    target.pushClipRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                        toPx(this->bounds.h));
    tinygpu::LinePrinter<RGB_T> printer;
    printer.setFont(*this->theme().typography.body);
    printer.setTarget(target);
    printer.setColor(this->theme().colors.onSurfaceVariant);
    printer.setTopBorder(toPx(cursorY));
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

  /// Screen routes every gesture here while the dialog is presented; only
  /// taps landing on an action are forwarded to it, everything else is
  /// swallowed (a modal dialog shouldn't leak touches through to whatever
  /// is behind it).
  bool onGesture(const tinygpu::GestureEvent& event) override {
    const int count = actionCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* action = actionAt(i);
      if (action->enabled && action->bounds.contains(event.point.x, event.point.y)) {
        return action->onGesture(event);
      }
    }
    return true;
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    const int count = actionCount();
    for (int i = 0; i < count; ++i) changed |= actionAt(i)->update(nowMs);
    return changed;
  }

  /// True if there's an action at (x, y) - this dialog's own (screen-space)
  /// coordinate space - that's draggable, recursing into it if it's itself
  /// a composite - see Widget::isDraggableAt().
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

  std::string title_;
  std::string message_;
  std::vector<Widget<RGB_T>*> actions_;
  ActionCountFn actionCountFn_;
  ActionAtFn actionAtFn_;
};

using DialogRGB565 = Dialog<tinygpu::RGB565>;
using DialogRGB666 = Dialog<tinygpu::RGB666>;
using DialogRGB888 = Dialog<tinygpu::RGB888>;

}  // namespace tinymd
