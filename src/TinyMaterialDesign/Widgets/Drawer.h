#pragma once
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Modal side navigation panel, holding a list of items (typically
 * ListItem).
 *
 * `bounds` is the panel's own rect - typically the left portion of the
 * screen, e.g. Bounds(0, 0, 220, kHeight) - not the full screen; like
 * Dialog, the scrim is drawn separately covering the whole target surface.
 * Show it with Screen::presentDialog(drawer) (Screen's modal-presentation
 * mechanism isn't specific to alert dialogs - any Widget works, including
 * this one) and dismiss it from an item's onClick or from onScrimTap.
 *
 *   Drawer<RGB565> drawer(Bounds(0, 0, 220, kHeight));
 *   ListItem<RGB565> radios(drawer.itemRect(0), "Radios", drawIcon);
 *   radios.onClick = []() { showRadios(); screen.dismissDialog(); };
 *   drawer.addItem(radios);
 *   drawer.onScrimTap = []() { screen.dismissDialog(); };
 *
 * Items (like Dialog's actions) are not owned by Drawer - position each
 * one's bounds (itemRect() suggests a spot) and add it with addItem().
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Drawer : public Widget<RGB_T> {
 public:
  Drawer() = default;
  explicit Drawer(Bounds bounds) { this->bounds = bounds; }

  /// Fired when a tap lands outside the panel (i.e. on the scrim) - wire
  /// this to Screen::dismissDialog() for tap-outside-to-close.
  std::function<void()> onScrimTap;

  void addItem(Widget<RGB_T>& item) {
    if (itemCount_ < kMaxItems) items_[itemCount_++] = &item;
  }

  /// Suggested rect for the `index`-th item, stacked from the panel's top.
  Bounds itemRect(int index, int32_t height = 48) const {
    return Bounds(this->bounds.x, this->bounds.y + index * height, this->bounds.w, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    const RGB_T scrim = blend(theme.colors.onBackground, RGB_T(0, 0, 0), 0.35f);
    target.fillRect(0, 0, target.width(), target.height(), scrim);

    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), theme.colors.surface);

    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->visible) items_[i]->draw(target, theme);
    }
  }

  /// Screen routes every gesture here while the drawer is presented, same
  /// as Dialog: taps on an item are forwarded to it; a tap outside the
  /// panel fires onScrimTap; everything else is swallowed rather than
  /// leaking through to whatever is behind the drawer.
  bool onGesture(const tinygpu::GestureEvent& event) override {
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->enabled && items_[i]->bounds.contains(event.point.x, event.point.y)) {
        return items_[i]->onGesture(event);
      }
    }
    if (isTapGesture(event.type) && !this->bounds.contains(event.point.x, event.point.y)) {
      if (onScrimTap) onScrimTap();
    }
    return true;
  }

  void update(uint32_t nowMs) override {
    for (int i = 0; i < itemCount_; ++i) items_[i]->update(nowMs);
  }

 private:
  static constexpr int kMaxItems = 8;

  Widget<RGB_T>* items_[kMaxItems] = {};
  int itemCount_ = 0;
};

using DrawerRGB565 = Drawer<tinygpu::RGB565>;
using DrawerRGB666 = Drawer<tinygpu::RGB666>;
using DrawerRGB888 = Drawer<tinygpu::RGB888>;

}  // namespace tinymd
