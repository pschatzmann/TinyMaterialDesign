#pragma once
#include <algorithm>
#include <cmath>
#include <functional>

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
 * Known limitation: like `Screen`'s own scrolling (see its class comment),
 * there's no sub-widget clip rect - an item scrolled partway past the
 * carousel's left/right edge still renders at full size for that moment
 * rather than being cropped. Cosmetic only.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Carousel : public Widget<RGB_T> {
 public:
  static constexpr int kMaxItems = 8;

  Carousel() = default;
  explicit Carousel(Bounds bounds, int32_t itemWidth = 140, int32_t gap = 12)
      : itemWidth_(itemWidth), gap_(gap) {
    this->bounds = bounds;
  }

  /// Fired whenever the settled current page changes - by drag-release snap
  /// or by setCurrentIndex().
  std::function<void(int)> onPageChange;

  /// Positions `item`'s own bounds (see class comment) and appends it.
  void addItem(Widget<RGB_T>& item) {
    if (itemCount_ >= kMaxItems) return;
    item.bounds = itemRect(itemCount_);
    items_[itemCount_++] = &item;
    if (this->theme_ != nullptr) item.setTheme(*this->theme_);
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i] != nullptr) items_[i]->setTheme(theme);
    }
  }

  /// Suggested content-space rect for the `index`-th item - see class
  /// comment. Leaves room at the bottom for the page-dot indicator.
  Bounds itemRect(int index) const {
    return Bounds(index * (itemWidth_ + gap_), 0, itemWidth_, this->bounds.h - kIndicatorHeight);
  }

  int currentIndex() const { return currentIndex_; }
  int pageCount() const { return itemCount_; }

  /// Pages to `index`, clamped to a valid item - animated (the same
  /// snap-glide update() uses after a drag release) unless `animate` is
  /// false.
  void setCurrentIndex(int index, bool animate = true) {
    if (itemCount_ == 0) return;
    index = std::min(std::max(index, 0), itemCount_ - 1);
    const bool changed = index != currentIndex_;
    currentIndex_ = index;
    targetOffsetX_ = static_cast<float>(index * (itemWidth_ + gap_));
    if (!animate) offsetX_ = targetOffsetX_;
    animating_ = animate && offsetX_ != targetOffsetX_;
    if (changed && onPageChange) onPageChange(currentIndex_);
  }

  bool isDraggable() const override { return true; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    for (int i = 0; i < itemCount_; ++i) {
      Widget<RGB_T>* item = items_[i];
      if (!item->visible) continue;
      const int32_t contentX = item->bounds.x;
      item->bounds.x = this->bounds.x + contentX - static_cast<int32_t>(offsetX_);
      if (item->bounds.right() > this->bounds.x && item->bounds.x < this->bounds.right()) {
        item->draw(target);
      }
      item->bounds.x = contentX;
    }
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
    for (int i = 0; i < itemCount_; ++i) {
      if (items_[i]->enabled && items_[i]->bounds.contains(contentX, event.point.y)) {
        tinygpu::GestureEvent shifted = event;
        shifted.point.x = static_cast<int16_t>(contentX);
        return items_[i]->onGesture(shifted);
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
    for (int i = 0; i < itemCount_; ++i) changed |= items_[i]->update(nowMs);
    return changed;
  }

 private:
  static constexpr int32_t kIndicatorHeight = 16;

  int32_t itemWidth_ = 140;
  int32_t gap_ = 12;
  Widget<RGB_T>* items_[kMaxItems] = {};
  int itemCount_ = 0;

  float offsetX_ = 0.0f;
  float targetOffsetX_ = 0.0f;
  bool animating_ = false;
  int currentIndex_ = 0;

  int32_t maxOffsetX() const {
    return itemCount_ > 0 ? std::max(int32_t{0}, (itemCount_ - 1) * (itemWidth_ + gap_)) : 0;
  }

  void clampOffset() {
    offsetX_ = std::min(static_cast<float>(maxOffsetX()), std::max(0.0f, offsetX_));
  }

  /// Rounds the current (mid-drag) offset to the nearest item and starts
  /// the snap-glide animation toward it - see update().
  void snapToNearest() {
    if (itemCount_ == 0) return;
    const int32_t step = itemWidth_ + gap_;
    const int index = step > 0 ? static_cast<int>(std::round(offsetX_ / step)) : 0;
    setCurrentIndex(index, /*animate=*/true);
  }

  void drawIndicator(tinygpu::ISurface<RGB_T>& target) {
    if (itemCount_ <= 1) return;
    constexpr int32_t kDotDiameter = 6;
    constexpr int32_t kDotGap = 8;
    const int32_t totalWidth = itemCount_ * kDotDiameter + (itemCount_ - 1) * kDotGap;
    int32_t x = this->bounds.centerX() - totalWidth / 2;
    const int32_t y = this->bounds.bottom() - kIndicatorHeight / 2;

    for (int i = 0; i < itemCount_; ++i) {
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
