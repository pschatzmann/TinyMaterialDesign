#pragma once
#include <functional>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/LinearLayout.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"  // IconPainter

namespace tinymd {

/// One NavigationBar/NavigationRail destination: an icon plus a short label.
template <typename RGB_T>
struct NavDestination {
  IconPainter<RGB_T> icon = nullptr;
  const char* label = "";
};

/**
 * @brief Bottom navigation bar: 3-5 exclusive destinations, each an icon
 * with a label underneath and a pill highlight behind the selected icon.
 *
 * Register with Screen::addFixedWidget() so it stays pinned to the bottom
 * of the viewport while scrollable content behind it scrolls.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class NavigationBar : public Widget<RGB_T> {
 public:
  NavigationBar() = default;
  explicit NavigationBar(Bounds bounds) { this->bounds = bounds; }

  /// Fired with the new selected index whenever a different destination is
  /// tapped.
  std::function<void(int)> onChange;

  void addDestination(IconPainter<RGB_T> icon, const char* label) {
    destinations_.push_back(NavDestination<RGB_T>{icon, label});
  }

  int selectedIndex() const { return selectedIndex_; }
  void setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(destinations_.size())) selectedIndex_ = index;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const int count = static_cast<int>(destinations_.size());
    if (count == 0) return;
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), this->theme().colors.surface);

    const LinearLayout slots(this->bounds, LayoutAxis::Horizontal, /*spacing=*/0);
    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;

    for (int i = 0; i < count; ++i) {
      const NavDestination<RGB_T>& dest = destinations_[i];
      const Bounds slot = slots.itemRect(i, count);
      const int32_t slotX = slot.x;
      const int32_t slotWidth = slot.w;
      const bool selected = i == selectedIndex_;
      RGB_T contentColor = selected ? this->theme().colors.onSecondaryContainer
                                    : this->theme().colors.onSurfaceVariant;
      if (!this->enabled) contentColor = blend(contentColor, this->theme().colors.surface, 0.5f);

      constexpr int32_t kIconSize = 24;
      constexpr int32_t kPillHeight = 32;
      const int32_t pillWidth = kIconSize + 24;
      const Bounds pill(slotX + slotWidth / 2 - pillWidth / 2, this->bounds.y + 8, pillWidth,
                        kPillHeight);
      if (selected) {
        target.fillRoundRect(toPx(pill.x), toPx(pill.y), toPx(pill.w), toPx(pill.h),
                             toPx(pill.h / 2), this->theme().colors.secondaryContainer);
      }

      if (dest.icon != nullptr) {
        const Bounds iconRect(pill.centerX() - kIconSize / 2, pill.centerY() - kIconSize / 2,
                              kIconSize, kIconSize);
        dest.icon(target, iconRect, contentColor, /*thickness=*/2);
      }

      const size_t labelWidth = font.measureTextWidth(dest.label);
      const int32_t labelX = slotX + slotWidth / 2 - static_cast<int32_t>(labelWidth) / 2;
      const int32_t labelY = pill.bottom() + 2;
      font.drawText(target, static_cast<int16_t>(labelX), static_cast<int16_t>(labelY), dest.label,
                   contentColor, this->theme().colors.surface, false);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    const int count = static_cast<int>(destinations_.size());
    if (!isTapGesture(event.type) || count == 0) return false;
    const int32_t slotWidth = this->bounds.w / count;
    int index = (event.point.x - this->bounds.x) / slotWidth;
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    if (index != selectedIndex_) {
      selectedIndex_ = index;
      if (onChange) onChange(selectedIndex_);
    }
    return true;
  }

 private:
  std::vector<NavDestination<RGB_T>> destinations_;
  int selectedIndex_ = 0;
};

using NavigationBarRGB565 = NavigationBar<tinygpu::RGB565>;
using NavigationBarRGB666 = NavigationBar<tinygpu::RGB666>;
using NavigationBarRGB888 = NavigationBar<tinygpu::RGB888>;

}  // namespace tinymd
