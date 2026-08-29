#pragma once
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Popover list of items (typically ListItem), anchored near whatever
 * opened it - e.g. a dropdown from a Button, or a context menu from a
 * long-press.
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
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Menu : public Widget<RGB_T> {
 public:
  Menu() = default;
  explicit Menu(Bounds bounds) { this->bounds = bounds; }

  /// Fired when a tap lands outside the panel - wire to
  /// Screen::dismissDialog() for tap-outside-to-close.
  std::function<void()> onOutsideTap;

  void addItem(Widget<RGB_T>& item) {
    if (itemCount_ < kMaxItems) {
      items_[itemCount_++] = &item;
      if (this->theme_ != nullptr) item.setTheme(*this->theme_);
    }
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i] != nullptr) items_[i]->setTheme(theme);
    }
  }

  /// Suggested rect for the `index`-th item, stacked from the panel's top.
  Bounds itemRect(int index, int32_t height = 40) const {
    return Bounds(this->bounds.x, this->bounds.y + index * height, this->bounds.w, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    drawBackground(target);
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->visible) items_[i]->draw(target);
    }
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

  int childCount() const override { return itemCount_; }
  Widget<RGB_T>* child(int index) override { return items_[index]; }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->enabled && items_[i]->bounds.contains(event.point.x, event.point.y)) {
        return items_[i]->onGesture(event);
      }
    }
    if (isTapGesture(event.type) && !this->bounds.contains(event.point.x, event.point.y)) {
      if (onOutsideTap) onOutsideTap();
    }
    return true;
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    for (int i = 0; i < itemCount_; ++i) changed |= items_[i]->update(nowMs);
    return changed;
  }

 private:
  static constexpr int kMaxItems = 8;

  Widget<RGB_T>* items_[kMaxItems] = {};
  int itemCount_ = 0;
};

using MenuRGB565 = Menu<tinygpu::RGB565>;
using MenuRGB666 = Menu<tinygpu::RGB666>;
using MenuRGB888 = Menu<tinygpu::RGB888>;

}  // namespace tinymd
