#pragma once
#include <functional>
#include <string>
#include <utility>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Container.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Modal panel sliding up from the bottom edge, full width, with
 * rounded top corners and a drag-handle bar - Material's "modal bottom
 * sheet". Holds a list of items (typically ListItem or Button) the same
 * non-owning way Drawer holds its items, in a Container (Core/Container.h)
 * that scrolls automatically once they overflow the sheet's height.
 *
 * `bounds` is the sheet's own rect (e.g. Bounds(0, screenH - 220, screenW,
 * 220)) - not the full screen; the scrim is drawn separately covering the
 * whole target surface, same convention as Dialog/Drawer. Show it with
 * Screen::presentDialog(sheet) and dismiss it from an item's onClick or
 * from onScrimTap.
 *
 * Known limitation: same as Drawer's - Screen::drawDirect()'s per-widget-
 * buffer rendering path draws items at their unscrolled position; only the
 * regular draw() path scrolls correctly. See Drawer.h's class comment.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class BottomSheet : public Widget<RGB_T> {
 public:
  BottomSheet() = default;
  explicit BottomSheet(Bounds bounds) {
    this->bounds = bounds;
    syncItemsBounds();
  }

  BottomSheet(Bounds bounds, const char* title) : title_(title != nullptr ? title : "") {
    this->bounds = bounds;
    syncItemsBounds();
  }

  void setTitle(const char* title) { title_ = title != nullptr ? title : ""; }

  /// Fired when a tap lands outside the panel (i.e. on the scrim).
  std::function<void()> onScrimTap;

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

  /// Suggested rect for the `index`-th item, stacked below the handle/title.
  /// Deliberately theme-independent (fixed constants, not
  /// theme().spacing/typography) - like Drawer::itemRect()/Menu::itemRect(),
  /// this is meant to be callable at global-scope construction time, before
  /// this sheet has ever been given a theme (see Drawer.h's class comment
  /// for that usage pattern).
  Bounds itemRect(int index, int32_t height = 48) const {
    return Bounds(this->bounds.x, contentTop() + index * height, this->bounds.w, height);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    drawBackground(target);
    syncItemsBounds();
    items_.draw(target);
  }

  /// Scrim + panel fill + handle + title, no items - see
  /// Widget::drawBackground() and childCount()/child() (same rationale as
  /// Drawer's own).
  void drawBackground(tinygpu::ISurface<RGB_T>& target) override {
    drawScrim(target);

    const size_t radius = toPx(this->theme().shape.large);
    // Rounded top corners only: draw a fully-rounded rect, then square off
    // the bottom half by overpainting it flat - cheaper than a 4-independent-
    // corner primitive, which ISurface doesn't expose.
    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.surface);
    const int32_t squareTop = this->bounds.y + static_cast<int32_t>(radius);
    if (squareTop < this->bounds.bottom()) {
      target.fillRect(toPx(this->bounds.x), toPx(squareTop), toPx(this->bounds.w),
                      toPx(this->bounds.bottom() - squareTop), this->theme().colors.surface);
    }

    constexpr int32_t kHandleWidth = 32;
    constexpr int32_t kHandleHeight = 4;
    const int32_t handleY = this->bounds.y + 8;
    target.fillRoundRect(toPx(this->bounds.centerX() - kHandleWidth / 2), toPx(handleY),
                         toPx(kHandleWidth), toPx(kHandleHeight), toPx(kHandleHeight / 2),
                         this->theme().colors.outline);

    if (!title_.empty()) {
      tinygpu::IFont<RGB_T>& font = *this->theme().typography.title;
      const int32_t pad = this->theme().spacing;
      font.drawText(target, static_cast<int16_t>(this->bounds.x + pad * 2),
                   static_cast<int16_t>(handleY + kHandleHeight + pad), title_.c_str(),
                   this->theme().colors.onSurface, this->theme().colors.surface, false);
    }
  }

  int childCount() const override { return items_.childCount(); }
  Widget<RGB_T>* child(int index) override { return items_.child(index); }

  /// A continuous drag starting over the item list scrolls it (see
  /// Container::onGesture()); a tap on an item is forwarded to it; a tap
  /// outside the panel fires onScrimTap; everything else is swallowed.
  bool onGesture(const tinygpu::GestureEvent& event) override {
    syncItemsBounds();

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
  int32_t contentTop() const {
    constexpr int32_t kHandleHeight = 4;
    constexpr int32_t kPad = 8;
    // Approximates the title typography's line height without touching
    // theme() (see itemRect()'s comment) - close enough for row layout, and
    // drawBackground() still measures the real font for the title itself.
    constexpr int32_t kApproxTitleHeight = 20;
    const int32_t afterHandle = this->bounds.y + 8 + kHandleHeight + kPad;
    if (title_.empty()) return afterHandle;
    return afterHandle + kApproxTitleHeight + kPad;
  }

  /// Keeps the item Container's own bounds in sync with this sheet's -
  /// items start below the handle/title, not at bounds.y itself.
  void syncItemsBounds() {
    const int32_t top = contentTop();
    items_.bounds = Bounds(this->bounds.x, top, this->bounds.w, this->bounds.bottom() - top);
  }

  std::string title_;
  Container<RGB_T> items_;
  bool routingToItems_ = false;
};

using BottomSheetRGB565 = BottomSheet<tinygpu::RGB565>;
using BottomSheetRGB666 = BottomSheet<tinygpu::RGB666>;
using BottomSheetRGB888 = BottomSheet<tinygpu::RGB888>;

}  // namespace tinymd
