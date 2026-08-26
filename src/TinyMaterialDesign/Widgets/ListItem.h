#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"

namespace tinymd {

/**
 * @brief Tappable row: optional leading icon + title, with a selected state.
 *
 * The building block for Drawer (Drawer.h), but usable standalone for any
 * settings-style list. Selected items draw as a filled pill (Material's
 * navigation-drawer look); unselected items are flat.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class ListItem : public Widget<RGB_T> {
 public:
  ListItem() = default;
  ListItem(Bounds bounds, const char* title, IconPainter<RGB_T> icon = nullptr)
      : title_(title), icon_(icon) {
    this->bounds = bounds;
  }

  std::function<void()> onClick;

  void setTitle(const char* title) { title_ = title; }
  void setIcon(IconPainter<RGB_T> icon) { icon_ = icon; }

  bool selected() const { return selected_; }
  void setSelected(bool selected) { selected_ = selected; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    RGB_T background = selected_ ? theme.colors.secondaryContainer : theme.colors.surface;
    RGB_T foreground = selected_ ? theme.colors.onSecondaryContainer : theme.colors.onSurfaceVariant;
    RGB_T textColor = selected_ ? theme.colors.onSecondaryContainer : theme.colors.onSurface;
    if (!this->enabled) {
      background = blend(background, theme.colors.surface, 0.5f);
      foreground = blend(foreground, theme.colors.surface, 0.5f);
      textColor = blend(textColor, theme.colors.surface, 0.5f);
    }

    const int32_t margin = 8;
    const Bounds pill = this->bounds.inset(margin);

    if (selected_) {
      target.fillRoundRect(toPx(pill.x), toPx(pill.y), toPx(pill.w), toPx(pill.h),
                           toPx(theme.shape.full), background);
    }

    const int32_t pad = 16;
    const int32_t iconSize = pill.h - 16;
    int32_t textX = pill.x + pad;
    if (icon_ != nullptr) {
      const Bounds iconRect(pill.x + pad, pill.y + (pill.h - iconSize) / 2, iconSize, iconSize);
      icon_(target, iconRect, foreground, /*thickness=*/2);
      textX = iconRect.right() + pad;
    }

    tinygpu::IFont<RGB_T>& font = *theme.typography.body;
    const int32_t textY = pill.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 title_.c_str(), textColor, background, false);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (onClick) onClick();
    return true;
  }

 private:
  std::string title_;
  IconPainter<RGB_T> icon_ = nullptr;
  bool selected_ = false;
};

using ListItemRGB565 = ListItem<tinygpu::RGB565>;
using ListItemRGB666 = ListItem<tinygpu::RGB666>;
using ListItemRGB888 = ListItem<tinygpu::RGB888>;

}  // namespace tinymd
