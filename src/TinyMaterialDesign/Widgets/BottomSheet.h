#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Modal panel sliding up from the bottom edge, full width, with
 * rounded top corners and a drag-handle bar - Material's "modal bottom
 * sheet". Holds a list of items (typically ListItem or Button) the same
 * non-owning way Drawer holds its items.
 *
 * `bounds` is the sheet's own rect (e.g. Bounds(0, screenH - 220, screenW,
 * 220)) - not the full screen; the scrim is drawn separately covering the
 * whole target surface, same convention as Dialog/Drawer. Show it with
 * Screen::presentDialog(sheet) and dismiss it from an item's onClick or
 * from onScrimTap.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class BottomSheet : public Widget<RGB_T> {
 public:
  BottomSheet() = default;
  explicit BottomSheet(Bounds bounds) { this->bounds = bounds; }

  BottomSheet(Bounds bounds, const char* title) : title_(title != nullptr ? title : "") {
    this->bounds = bounds;
  }

  void setTitle(const char* title) { title_ = title != nullptr ? title : ""; }

  /// Fired when a tap lands outside the panel (i.e. on the scrim).
  std::function<void()> onScrimTap;

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
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->visible) items_[i]->draw(target);
    }
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

  int childCount() const override { return itemCount_; }
  Widget<RGB_T>* child(int index) override { return items_[index]; }

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

  bool update(uint32_t nowMs) override {
    bool changed = false;
    for (int i = 0; i < itemCount_; ++i) changed |= items_[i]->update(nowMs);
    return changed;
  }

 private:
  static constexpr int kMaxItems = 8;

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

  std::string title_;
  Widget<RGB_T>* items_[kMaxItems] = {};
  int itemCount_ = 0;
};

using BottomSheetRGB565 = BottomSheet<tinygpu::RGB565>;
using BottomSheetRGB666 = BottomSheet<tinygpu::RGB666>;
using BottomSheetRGB888 = BottomSheet<tinygpu::RGB888>;

}  // namespace tinymd
