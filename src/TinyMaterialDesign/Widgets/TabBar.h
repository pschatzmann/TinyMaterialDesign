#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// Row of evenly-spaced exclusive-selection labels (primary tabs), with a
/// sliding indicator under the selected one. Tabs are fixed text strings
/// set via setTabs()/addTab() - up to kMaxTabs - rather than separate child
/// widgets, so the whole row is one hit-testable/drawable Widget.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class TabBar : public Widget<RGB_T> {
 public:
  static constexpr int kMaxTabs = 6;

  TabBar() = default;
  explicit TabBar(Bounds bounds) { this->bounds = bounds; }

  /// Fired with the new selected index whenever a different tab is tapped.
  std::function<void(int)> onChange;

  void addTab(const char* label) {
    if (tabCount_ < kMaxTabs) tabs_[tabCount_++] = label;
  }
  void clearTabs() { tabCount_ = 0; }

  int selectedIndex() const { return selectedIndex_; }
  void setSelectedIndex(int index) {
    if (index >= 0 && index < tabCount_) selectedIndex_ = index;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    if (tabCount_ == 0) return;
    const int32_t tabWidth = this->bounds.w / tabCount_;
    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;

    for (int i = 0; i < tabCount_; ++i) {
      const int32_t x = this->bounds.x + i * tabWidth;
      const bool selected = i == selectedIndex_;
      RGB_T textColor = selected ? this->theme().colors.primary : this->theme().colors.onSurfaceVariant;
      if (!this->enabled) textColor = blend(textColor, this->theme().colors.surface, 0.5f);

      const size_t textWidth = font.measureTextWidth(tabs_[i]);
      const int32_t textX = x + tabWidth / 2 - static_cast<int32_t>(textWidth) / 2;
      const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), tabs_[i],
                   textColor, this->theme().colors.surface, false);
    }

    constexpr int32_t kIndicatorHeight = 3;
    const int32_t indicatorX = this->bounds.x + selectedIndex_ * tabWidth;
    RGB_T indicatorColor =
        this->enabled ? this->theme().colors.primary
                      : blend(this->theme().colors.primary, this->theme().colors.surface, 0.5f);
    target.fillRect(toPx(indicatorX), toPx(this->bounds.bottom() - kIndicatorHeight), toPx(tabWidth),
                    toPx(kIndicatorHeight), indicatorColor);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type) || tabCount_ == 0) return false;
    const int32_t tabWidth = this->bounds.w / tabCount_;
    int index = (event.point.x - this->bounds.x) / tabWidth;
    if (index < 0) index = 0;
    if (index >= tabCount_) index = tabCount_ - 1;
    if (index != selectedIndex_) {
      selectedIndex_ = index;
      if (onChange) onChange(selectedIndex_);
    }
    return true;
  }

 private:
  const char* tabs_[kMaxTabs] = {};
  int tabCount_ = 0;
  int selectedIndex_ = 0;
};

using TabBarRGB565 = TabBar<tinygpu::RGB565>;
using TabBarRGB666 = TabBar<tinygpu::RGB666>;
using TabBarRGB888 = TabBar<tinygpu::RGB888>;

}  // namespace tinymd
