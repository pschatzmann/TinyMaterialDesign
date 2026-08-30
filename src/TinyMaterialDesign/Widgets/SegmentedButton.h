#pragma once
#include <functional>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/LinearLayout.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// @brief Connected row of segments sharing one pill-shaped outline (M3 "segmented
/// button"). Single-select (radio-like, the default) or multi-select
/// (checkbox-like, toggling independently) - see setMultiSelect().
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class SegmentedButton : public Widget<RGB_T> {
 public:
  SegmentedButton() = default;
  explicit SegmentedButton(Bounds bounds) { this->bounds = bounds; }

  /// Fired whenever selection changes, with a bitmask of selected segments
  /// (bit i set == segment i selected) - a single-select bar always has
  /// exactly one bit set. Limits a bar to at most 32 segments (the bitmask's
  /// width), well beyond what's usable as an actual row of segments.
  std::function<void(uint32_t)> onChange;

  void addSegment(const char* label) { segments_.push_back(label); }
  void setMultiSelect(bool multiSelect) { multiSelect_ = multiSelect; }

  bool isSelected(int index) const { return (selectedMask_ & (1u << index)) != 0; }
  void setSelected(int index, bool selected) {
    if (index < 0 || index >= static_cast<int>(segments_.size())) return;
    if (!multiSelect_) {
      selectedMask_ = selected ? (1u << index) : 0u;
      return;
    }
    if (selected) {
      selectedMask_ |= (1u << index);
    } else {
      selectedMask_ &= ~(1u << index);
    }
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const int count = static_cast<int>(segments_.size());
    if (count == 0) return;
    const LinearLayout slots(this->bounds, LayoutAxis::Horizontal, /*spacing=*/0);
    const size_t radius = toPx(this->theme().shape.full);
    RGB_T outline = this->theme().colors.outline;
    if (!this->enabled) outline = blend(outline, this->theme().colors.surface, 0.5f);

    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, outline);

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.label;
    for (int i = 0; i < count; ++i) {
      const Bounds slot = slots.itemRect(i, count);
      const int32_t x = slot.x;
      const int32_t segmentWidth = slot.w;
      const bool selected = isSelected(i);
      RGB_T background = selected ? this->theme().colors.secondaryContainer : this->theme().colors.surface;
      RGB_T foreground =
          selected ? this->theme().colors.onSecondaryContainer : this->theme().colors.onSurfaceVariant;
      if (!this->enabled) {
        background = blend(background, this->theme().colors.surface, 0.5f);
        foreground = blend(foreground, this->theme().colors.surface, 0.5f);
      }

      if (selected) {
        // Inset by 1px so the selected fill sits inside the shared outline
        // rather than painting over it.
        target.fillRect(toPx(x + 1), toPx(this->bounds.y + 1), toPx(segmentWidth - 1),
                        toPx(this->bounds.h - 2), background);
      }
      if (i > 0) {
        target.drawLine(toPx(x), toPx(this->bounds.y), toPx(x), toPx(this->bounds.bottom()), outline);
      }

      const size_t textWidth = font.measureTextWidth(segments_[i]);
      const int32_t textX = x + segmentWidth / 2 - static_cast<int32_t>(textWidth) / 2;
      const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), segments_[i],
                   foreground, background, false);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    const int count = static_cast<int>(segments_.size());
    if (!isTapGesture(event.type) || count == 0) return false;
    const int32_t segmentWidth = this->bounds.w / count;
    int index = (event.point.x - this->bounds.x) / segmentWidth;
    if (index < 0 || index >= count) return true;

    if (multiSelect_) {
      setSelected(index, !isSelected(index));
    } else {
      selectedMask_ = 1u << index;
    }
    if (onChange) onChange(selectedMask_);
    return true;
  }

 private:
  std::vector<const char*> segments_;
  uint32_t selectedMask_ = 1u;  // segment 0 selected by default
  bool multiSelect_ = false;
};

using SegmentedButtonRGB565 = SegmentedButton<tinygpu::RGB565>;
using SegmentedButtonRGB666 = SegmentedButton<tinygpu::RGB666>;
using SegmentedButtonRGB888 = SegmentedButton<tinygpu::RGB888>;

}  // namespace tinymd
