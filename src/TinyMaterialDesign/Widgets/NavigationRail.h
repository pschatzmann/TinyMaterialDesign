#pragma once
#include <algorithm>
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/NavigationBar.h"  // NavDestination

namespace tinymd {

/**
 * @brief Side navigation rail: a narrow vertical column of 3-7 exclusive
 * destinations, each an icon with a label underneath and a pill highlight
 * behind the selected icon - the tablet/landscape counterpart of
 * NavigationBar.
 *
 * Register with Screen::addFixedWidget() so it stays pinned to the side of
 * the viewport while scrollable content behind it scrolls.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class NavigationRail : public Widget<RGB_T> {
 public:
  static constexpr int kMaxDestinations = 7;

  NavigationRail() = default;
  explicit NavigationRail(Bounds bounds) { this->bounds = bounds; }

  std::function<void(int)> onChange;

  void addDestination(IconPainter<RGB_T> icon, const char* label) {
    if (destCount_ < kMaxDestinations) destinations_[destCount_++] = NavDestination<RGB_T>{icon, label};
  }

  int selectedIndex() const { return selectedIndex_; }
  void setSelectedIndex(int index) {
    if (index >= 0 && index < destCount_) selectedIndex_ = index;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    if (destCount_ == 0) return;
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), this->theme().colors.surface);

    const int32_t slotHeight = this->bounds.h / destCount_;
    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;

    for (int i = 0; i < destCount_; ++i) {
      const NavDestination<RGB_T>& dest = destinations_[i];
      const int32_t slotY = this->bounds.y + i * slotHeight;
      const bool selected = i == selectedIndex_;
      RGB_T contentColor = selected ? this->theme().colors.onSecondaryContainer
                                    : this->theme().colors.onSurfaceVariant;
      if (!this->enabled) contentColor = blend(contentColor, this->theme().colors.surface, 0.5f);

      constexpr int32_t kIconSize = 24;
      constexpr int32_t kPillHeight = 32;
      const int32_t pillWidth = std::min<int32_t>(this->bounds.w - 16, kIconSize + 24);
      const Bounds pill(this->bounds.centerX() - pillWidth / 2, slotY + slotHeight / 2 - kPillHeight / 2 - 6,
                        pillWidth, kPillHeight);
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
      const int32_t labelX = this->bounds.centerX() - static_cast<int32_t>(labelWidth) / 2;
      const int32_t labelY = pill.bottom() + 2;
      font.drawText(target, static_cast<int16_t>(labelX), static_cast<int16_t>(labelY), dest.label,
                   contentColor, this->theme().colors.surface, false);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type) || destCount_ == 0) return false;
    const int32_t slotHeight = this->bounds.h / destCount_;
    int index = (event.point.y - this->bounds.y) / slotHeight;
    if (index < 0) index = 0;
    if (index >= destCount_) index = destCount_ - 1;
    if (index != selectedIndex_) {
      selectedIndex_ = index;
      if (onChange) onChange(selectedIndex_);
    }
    return true;
  }

 private:
  NavDestination<RGB_T> destinations_[kMaxDestinations] = {};
  int destCount_ = 0;
  int selectedIndex_ = 0;
};

using NavigationRailRGB565 = NavigationRail<tinygpu::RGB565>;
using NavigationRailRGB666 = NavigationRail<tinygpu::RGB666>;
using NavigationRailRGB888 = NavigationRail<tinygpu::RGB888>;

}  // namespace tinymd
