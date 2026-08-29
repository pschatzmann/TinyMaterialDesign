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
 * @brief Modal side navigation panel, holding a list of items (typically
 * ListItem) that scrolls automatically once they overflow the panel's
 * height - the item area is a Container (Core/Container.h) under the hood.
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
 *
 * Alternative to addItem(): setItemProvider(count, at) switches to
 * callback-driven items - see Container.h's class comment for the pattern
 * (useful for a long settings-style list too large to keep every ListItem
 * resident at once).
 *
 * Known limitation: Screen::drawDirect()'s per-widget-buffer rendering path
 * (see childCount()/child() and Screen.h's own class comment) draws each
 * item at its authored, unscrolled position regardless of the current
 * scroll offset - correct only while the drawer isn't scrolled. The regular
 * draw() path (used by the common draw()+writeData(surface) loop) scrolls
 * correctly in both cases; only the rare low-RAM direct-render path has
 * this gap.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Drawer : public Widget<RGB_T> {
 public:
  Drawer() = default;
  explicit Drawer(Bounds bounds) {
    this->bounds = bounds;
    items_.bounds = bounds;
  }

  /// Fired when a tap lands outside the panel (i.e. on the scrim) - wire
  /// this to Screen::dismissDialog() for tap-outside-to-close.
  std::function<void()> onScrimTap;

  void addItem(Widget<RGB_T>& item) { items_.addChild(item); }

  /// Switches to callback-driven items - see the class comment.
  void setItemProvider(typename Container<RGB_T>::ChildCountFn count,
                       typename Container<RGB_T>::ChildAtFn at) {
    items_.setChildProvider(std::move(count), std::move(at));
  }

  /// Cascades the theme down to every item already added - see addItem().
  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    items_.setTheme(theme);
  }

  /// Suggested rect for the `index`-th item, stacked from the panel's top.
  Bounds itemRect(int index, int32_t height = 48) const {
    return Bounds(this->bounds.x, this->bounds.y + index * height, this->bounds.w, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    drawBackground(target);
    items_.bounds = this->bounds;
    items_.draw(target);
  }

  /// Scrim + panel fill only, no items - see Widget::drawBackground() and
  /// childCount()/child() below, which together let
  /// Screen::drawDirect() draw this (on a board without enough RAM for a
  /// full-screen buffer) as this small background plus each item
  /// individually, rather than needing one buffer sized to the whole
  /// drawer (its own bounds are often nearly full-screen).
  void drawBackground(tinygpu::ISurface<RGB_T>& target) override {
    drawScrim(target);

    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), this->theme().colors.surface);
  }

  int childCount() const override { return items_.childCount(); }
  Widget<RGB_T>* child(int index) override { return items_.child(index); }

  /// Screen routes every gesture here while the drawer is presented: a
  /// continuous drag starting over the item list scrolls it (see
  /// Container::onGesture()); a tap on an item is forwarded to it; a tap
  /// outside the panel fires onScrimTap; everything else is swallowed
  /// rather than leaking through to whatever is behind the drawer.
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
      if (onScrimTap) onScrimTap();
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

using DrawerRGB565 = Drawer<tinygpu::RGB565>;
using DrawerRGB666 = Drawer<tinygpu::RGB666>;
using DrawerRGB888 = Drawer<tinygpu::RGB888>;

}  // namespace tinymd
