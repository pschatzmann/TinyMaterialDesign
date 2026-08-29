#pragma once
#include <functional>
#include <utility>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Container.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Popover list of items (typically ListItem), anchored near whatever
 * opened it - e.g. a dropdown from a Button, or a context menu from a
 * long-press. Scrolls automatically once the items overflow the panel's
 * height - the item area is a Container (Core/Container.h) under the hood.
 *
 * Same modal-presentation shape as Dialog/Drawer: show with
 * Screen::presentDialog(menu) and dismiss from an item's onClick or
 * onOutsideTap. Unlike Dialog/Drawer it does *not* dim the rest of the
 * screen (a menu is a lightweight popover, not a heavyweight interruption)
 * - it just draws its own panel and swallows every gesture outside it.
 *
 *   Menu<RGB565> menu(Bounds(140, 60, 160, 3 * 40));
 *   ListItem<RGB565> item(menu.itemRect(0), "Rename");
 *   item.onClick = []() { doRename(); screen.dismissDialog(); };
 *   menu.addItem(item);
 *   menu.onOutsideTap = []() { screen.dismissDialog(); };
 *   screen.presentDialog(menu);
 *
 * Items are not owned by Menu - same non-owning addItem() convention as
 * Drawer::addItem()/Dialog::addAction().
 *
 * Known limitation: same as Drawer's - Screen::drawDirect()'s per-widget-
 * buffer rendering path draws items at their unscrolled position; only the
 * regular draw() path scrolls correctly. See Drawer.h's class comment.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Menu : public Widget<RGB_T> {
 public:
  Menu() = default;
  explicit Menu(Bounds bounds) {
    this->bounds = bounds;
    items_.bounds = bounds;
  }

  /// Fired when a tap lands outside the panel - wire to
  /// Screen::dismissDialog() for tap-outside-to-close.
  std::function<void()> onOutsideTap;

  void addItem(Widget<RGB_T>& item) { items_.addChild(item); }

  /// Switches to callback-driven items - see Container.h's class comment
  /// and Drawer.h's own setItemProvider() for the same pattern.
  void setItemProvider(typename Container<RGB_T>::ChildCountFn count,
                       typename Container<RGB_T>::ChildAtFn at) {
    items_.setChildProvider(std::move(count), std::move(at));
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    items_.setTheme(theme);
  }

  /// Suggested rect for the `index`-th item, stacked from the panel's top.
  Bounds itemRect(int index, int32_t height = 40) const {
    return Bounds(this->bounds.x, this->bounds.y + index * height, this->bounds.w, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    drawBackground(target);
    items_.bounds = this->bounds;
    items_.draw(target);
  }

  /// Panel fill only, no items - see Widget::drawBackground() and
  /// childCount()/child() below (same rationale as Drawer's).
  void drawBackground(tinygpu::ISurface<RGB_T>& target) override {
    const size_t radius = toPx(this->theme().shape.medium);
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.surface);
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.outline);
  }

  int childCount() const override { return items_.childCount(); }
  Widget<RGB_T>* child(int index) override { return items_.child(index); }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    items_.bounds = this->bounds;

    if (Container<RGB_T>::isContinuousType(event.type)) {
      if (event.phase == tinygpu::GesturePhase::kBegan) {
        routingToItems_ = items_.bounds.contains(event.startPoint.x, event.startPoint.y);
      }
      return routingToItems_ ? items_.onGesture(event) : true;
    }

    if (items_.bounds.contains(event.point.x, event.point.y) && items_.onGesture(event)) {
      return true;
    }
    if (isTapGesture(event.type) && !this->bounds.contains(event.point.x, event.point.y)) {
      if (onOutsideTap) onOutsideTap();
    }
    return true;
  }

  bool update(uint32_t nowMs) override { return items_.update(nowMs); }

  /// True if there's an item at (x, y) that's draggable, recursing through
  /// the item Container - see Widget::isDraggableAt().
  bool isDraggableAt(int32_t x, int32_t y) const override {
    return items_.bounds.contains(x, y) && items_.isDraggableAt(x, y);
  }

 private:
  Container<RGB_T> items_;
  bool routingToItems_ = false;
};

using MenuRGB565 = Menu<tinygpu::RGB565>;
using MenuRGB666 = Menu<tinygpu::RGB666>;
using MenuRGB888 = Menu<tinygpu::RGB888>;

}  // namespace tinymd
