#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Horizontally paged, drag-to-swipe row of items (typically
 * `MediaCard`), with a snap-to-item animation and a page-dot indicator -
 * Material's "carousel", e.g. a row of featured stations/genres.
 *
 * Items are not owned by Carousel - same non-owning `addItem()` convention
 * as Drawer/BottomSheet. Each item's `bounds` is authored in the carousel's
 * own *content* space (see `itemRect()`, stacked left-to-right by
 * `itemWidth + gap`, starting at x=0) rather than screen space - `draw()`
 * temporarily shifts each item's `bounds.x` by the carousel's own position
 * and current scroll offset before drawing it, then restores it, the same
 * pattern `Screen::draw()` uses for its own vertically-scrolled content.
 * `onGesture()` translates a tap's point the same way before forwarding it
 * to whichever item it landed on.
 *
 * Reports `isDraggable() == true` (like `Slider`) so a horizontal drag
 * starting on the carousel is classified as `GestureType::kDrag` and
 * latched to it for the drag's whole lifetime - see `Screen::
 * isDraggableAt()`. Releasing mid-drag snaps to the nearest item,
 * animated over a few frames via `update()`.
 *
 * An item scrolled partway past the carousel's left/right edge is cropped
 * to it (see ISurface::pushClipRect()), not drawn in full or skipped.
 *
 * Alternative to addItem(): setItemProvider(count, at) switches to
 * callback-driven items, for a station/genre list too large to keep every
 * MediaCard resident at once - see Container.h's class comment for the same
 * pattern. `at(index)`'s returned Widget has its bounds overwritten with
 * itemRect(index) on every fetch (addItem() only does this once, since a
 * vector-mode item's position never needs to change again).
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Carousel : public Widget<RGB_T> {
 public:
  using ItemCountFn = std::function<int()>;
  using ItemAtFn = std::function<Widget<RGB_T>&(int index)>;

  Carousel() = default;
  explicit Carousel(Bounds bounds, int32_t itemWidth = 140, int32_t gap = 12)
      : itemWidth_(itemWidth), gap_(gap) {
    this->bounds = bounds;
  }

  /// Fired whenever the settled current page changes - by drag-release snap
  /// or by setCurrentIndex().
  std::function<void(int)> onPageChange;

  /// Positions `item`'s own bounds (see class comment) and appends it.
  /// Ignored while an item provider is active - see setItemProvider().
  void addItem(Widget<RGB_T>& item) {
    item.bounds = itemRect(static_cast<int>(items_.size()));
    items_.push_back(&item);
    if (this->theme_ != nullptr) item.setTheme(*this->theme_);
  }

  /// Switches to callback-driven items - see the class comment.
  void setItemProvider(ItemCountFn count, ItemAtFn at) {
    itemCountFn_ = std::move(count);
    itemAtFn_ = std::move(at);
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (Widget<RGB_T>* item : items_) {
      if (item != nullptr) item->setTheme(theme);
    }
  }

  /// Suggested content-space rect for the `index`-th item - see class
  /// comment. Leaves room at the bottom for the page-dot indicator.
  Bounds itemRect(int index) const {
    return Bounds(index * (itemWidth_ + gap_), 0, itemWidth_, this->bounds.h - kIndicatorHeight);
  }

  int currentIndex() const { return currentIndex_; }
  int pageCount() const { return itemCount(); }

  /// Pages to `index`, clamped to a valid item - animated (the same
  /// snap-glide update() uses after a drag release) unless `animate` is
  /// false.
  void setCurrentIndex(int index, bool animate = true) {
    const int count = itemCount();
    if (count == 0) return;
    index = std::min(std::max(index, 0), count - 1);
    const bool changed = index != currentIndex_;
    currentIndex_ = index;
    targetOffsetX_ = static_cast<float>(index * (itemWidth_ + gap_));
    if (!animate) offsetX_ = targetOffsetX_;
    animating_ = animate && offsetX_ != targetOffsetX_;
    if (changed && onPageChange) onPageChange(currentIndex_);
  }

  bool isDraggable() const override { return true; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    target.pushClipRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                        toPx(this->bounds.h));
    const int count = itemCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* item = itemAt(i);
      if (!item->visible) continue;
      const int32_t contentX = item->bounds.x;
      item->bounds.x = this->bounds.x + contentX - static_cast<int32_t>(offsetX_);
      if (item->bounds.right() > this->bounds.x && item->bounds.x < this->bounds.right()) {
        item->draw(target);
      }
      item->bounds.x = contentX;
    }
    target.popClipRect();
    drawIndicator(target);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (event.type == tinygpu::GestureType::kDrag) {
      if (event.phase == tinygpu::GesturePhase::kBegan) animating_ = false;
      offsetX_ -= event.stepDeltaX;
      clampOffset();
      if (event.phase == tinygpu::GesturePhase::kEnded) snapToNearest();
      return true;
    }
    if (!isTapGesture(event.type)) return false;

    const int32_t contentX = event.point.x - this->bounds.x + static_cast<int32_t>(offsetX_);
    const int count = itemCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* item = itemAt(i);
      if (item->enabled && item->bounds.contains(contentX, event.point.y)) {
        tinygpu::GestureEvent shifted = event;
        shifted.point.x = static_cast<int16_t>(contentX);
        return item->onGesture(shifted);
      }
    }
    return true;
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    if (animating_) {
      const float diff = targetOffsetX_ - offsetX_;
      if (std::fabs(diff) < 1.0f) {
        offsetX_ = targetOffsetX_;
        animating_ = false;
      } else {
        offsetX_ += diff * 0.3f;
      }
      changed = true;
    }
    const int count = itemCount();
    for (int i = 0; i < count; ++i) changed |= itemAt(i)->update(nowMs);
    return changed;
  }

 private:
  static constexpr int32_t kIndicatorHeight = 16;

  int32_t itemWidth_ = 140;
  int32_t gap_ = 12;
  std::vector<Widget<RGB_T>*> items_;
  ItemCountFn itemCountFn_;
  ItemAtFn itemAtFn_;

  float offsetX_ = 0.0f;
  float targetOffsetX_ = 0.0f;
  bool animating_ = false;
  int currentIndex_ = 0;

  int itemCount() const { return itemCountFn_ ? itemCountFn_() : static_cast<int>(items_.size()); }

  /// The item at `index` right now - from the provider if active (bounds
  /// reset to itemRect(index) and themed on every fetch, since a pooled
  /// provider Widget may have just served a different index - see the class
  /// comment), else from the addItem()'d vector (bounds set once, at add
  /// time).
  Widget<RGB_T>* itemAt(int index) const {
    if (itemAtFn_) {
      Widget<RGB_T>& item = itemAtFn_(index);
      item.bounds = itemRect(index);
      if (this->theme_ != nullptr) item.setTheme(*this->theme_);
      return &item;
    }
    return items_[static_cast<size_t>(index)];
  }

  int32_t maxOffsetX() const {
    const int count = itemCount();
    return count > 0 ? std::max(int32_t{0}, (count - 1) * (itemWidth_ + gap_)) : 0;
  }

  void clampOffset() {
    offsetX_ = std::min(static_cast<float>(maxOffsetX()), std::max(0.0f, offsetX_));
  }

  /// Rounds the current (mid-drag) offset to the nearest item and starts
  /// the snap-glide animation toward it - see update().
  void snapToNearest() {
    if (itemCount() == 0) return;
    const int32_t step = itemWidth_ + gap_;
    const int index = step > 0 ? static_cast<int>(std::round(offsetX_ / step)) : 0;
    setCurrentIndex(index, /*animate=*/true);
  }

  void drawIndicator(tinygpu::ISurface<RGB_T>& target) {
    const int count = itemCount();
    if (count <= 1) return;
    constexpr int32_t kDotDiameter = 6;
    constexpr int32_t kDotGap = 8;
    const int32_t totalWidth = count * kDotDiameter + (count - 1) * kDotGap;
    int32_t x = this->bounds.centerX() - totalWidth / 2;
    const int32_t y = this->bounds.bottom() - kIndicatorHeight / 2;

    for (int i = 0; i < count; ++i) {
      const RGB_T color =
          i == currentIndex_ ? this->theme().colors.primary : this->theme().colors.surfaceVariant;
      target.fillCircle(toPx(x + kDotDiameter / 2), toPx(y), toPx(kDotDiameter / 2), color);
      x += kDotDiameter + kDotGap;
    }
  }
};

using CarouselRGB565 = Carousel<tinygpu::RGB565>;
using CarouselRGB666 = Carousel<tinygpu::RGB666>;
using CarouselRGB888 = Carousel<tinygpu::RGB888>;

}  // namespace tinymd
