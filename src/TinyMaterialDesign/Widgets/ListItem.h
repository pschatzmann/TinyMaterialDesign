#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"
#include "TinyMaterialDesign/Widgets/Label.h"  // TypographyRole, fontForTypographyRole()

namespace tinymd {

/**
 * @brief List row after the Material 3 list spec: a leading avatar or icon,
 * a headline, an optional supporting text, a trailing text or icon, and an
 * optional divider. Selected rows draw as a filled pill (the navigation-drawer
 * look). Everything beyond the headline is opt-in, so a plain ListItem looks
 * as before.
 *
 *   ListItem<RGB565> row(Bounds(0, y, 240, ListItem<RGB565>::kOneLineHeight), "Quarterly report");
 *   row.setSupportingText("Alice <alice@example.com>");   // two-line item
 *   row.setLeadingText("A");                              // 40 dp avatar with a letter
 *   row.setTrailingText("2 files");                       // label on the right
 *   row.setDivider(true);                                 // divider at the bottom edge
 *
 * Sizes (dp, the same as px here): one-line item 56, two-line item 72, avatar
 * 40, icon 24, dividers 1 with an inset of 16 from the text.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class ListItem : public Widget<RGB_T> {
 public:
  /// Height of a one-line item (headline only), Material 3
  static constexpr int32_t kOneLineHeight = 56;
  /// Height of a two-line item (headline and supporting text), Material 3
  static constexpr int32_t kTwoLineHeight = 72;

  ListItem() = default;
  ListItem(Bounds bounds, const char* title, IconPainter<RGB_T> icon = nullptr)
      : title_(title), icon_(icon) {
    this->bounds = bounds;
  }

  std::function<void()> onClick;

  // ---- headline and supporting text -------------------------------------------
  void setTitle(const char* title) { title_ = title != nullptr ? title : ""; }

  /// Supporting text under the headline (a two-line item), in the label role.
  void setSupportingText(const char* text) { supporting_ = text != nullptr ? text : ""; }
  /// Same as setSupportingText() (earlier name)
  void setSubtitle(const char* subtitle) { setSupportingText(subtitle); }

  /// Text size of the headline - kBody (the default) down to kLabel.
  void setTypographyRole(TypographyRole role) { role_ = role; }

  /// Fonts of the headline and of the supporting text (default: the fonts of the
  /// typography roles of the theme). The text is clipped to the row.
  void setTitleFont(tinygpu::IFont<RGB_T>& font) { titleFont_ = &font; }
  void setSubtitleFont(tinygpu::IFont<RGB_T>& font) { subtitleFont_ = &font; }

  /// Colour of the headline (default: the text colour of the row).
  void setTitleColor(RGB_T color) {
    titleColor_ = color;
    hasTitleColor_ = true;
  }

  // ---- leading: avatar with a letter, or an icon ------------------------------
  /// A 40 dp circle with this text (typically one letter) at the start of the row.
  void setLeadingText(const char* text) { leadingText_ = text != nullptr ? text : ""; }
  /// A 24 dp icon at the start of the row (ignored when there is a leading text).
  void setIcon(IconPainter<RGB_T> icon) { icon_ = icon; }

  // ---- trailing: text or a 24 dp icon at the end of the row -------------------
  void setTrailingText(const char* text) { trailingText_ = text != nullptr ? text : ""; }
  void setTrailingIcon(IconPainter<RGB_T> icon) { trailingIcon_ = icon; }

  // ---- dividers ------------------------------------------------------------------
  /// A 1 dp line along the bottom edge of the row.
  void setDivider(bool divider) { divider_ = divider; }
  /// true: the divider starts after the leading slot (inset); false: full width.
  void setDividerInset(bool inset) { dividerInset_ = inset; }
  /// Colour of the divider (default: the outline colour of the theme). A divider in
  /// the page background colour reads as a gap between two filled items.
  void setDividerColor(RGB_T color) {
    dividerColor_ = color;
    hasDividerColor_ = true;
  }

  // ---- fill ----------------------------------------------------------------------
  /// Fills the whole row with a rounded rectangle of this colour (the row's bounds
  /// give its width, so a narrower, centred row is a card). Selected rows keep
  /// their own colour.
  void setBackgroundColor(RGB_T color) {
    backgroundColor_ = color;
    hasBackgroundColor_ = true;
  }
  void clearBackgroundColor() { hasBackgroundColor_ = false; }

  // ---- selection and state ----------------------------------------------------
  bool selected() const { return selected_; }
  void setSelected(bool selected) { selected_ = selected; }

  /// Height the row needs: a two-line item when it has supporting text.
  int32_t preferredHeight() const {
    return supporting_.empty() ? kOneLineHeight : kTwoLineHeight;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    RGB_T background = selected_ ? t.colors.secondaryContainer : t.colors.surface;
    RGB_T foreground = selected_ ? t.colors.onSecondaryContainer : t.colors.onSurfaceVariant;
    RGB_T textColor = selected_ ? t.colors.onSecondaryContainer : t.colors.onSurface;
    RGB_T outline = hasDividerColor_ ? dividerColor_ : t.colors.outline;
    if (hasBackgroundColor_ && !selected_) {
      background = backgroundColor_;
      target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                           toPx(this->bounds.h), toPx(t.shape.medium), backgroundColor_);
    }
    if (!this->enabled) {
      background = blend(background, t.colors.surface, 0.5f);
      foreground = blend(foreground, t.colors.surface, 0.5f);
      textColor = blend(textColor, t.colors.surface, 0.5f);
      outline = blend(outline, t.colors.surface, 0.5f);
    }

    const int32_t margin = 8;
    const Bounds pill = this->bounds.inset(margin);

    if (selected_) {
      target.fillRoundRect(toPx(pill.x), toPx(pill.y), toPx(pill.w), toPx(pill.h),
                           toPx(t.shape.full), background);
    }

    const int32_t pad = 16;
    const int32_t avatarSize = pill.h < 40 ? pill.h : 40;  // 40 dp avatar
    const int32_t iconSize = 24;                           // 24 dp icon
    int32_t textX = pill.x + pad;

    // ---- leading slot
    if (!leadingText_.empty()) {
      const int32_t r = avatarSize / 2;
      const int32_t cx = pill.x + pad + r;
      const int32_t cy = pill.centerY();
      target.fillCircle(static_cast<size_t>(cx), static_cast<size_t>(cy), static_cast<size_t>(r),
                        t.colors.primaryContainer);
      tinygpu::IFont<RGB_T>& letters = *fontForTypographyRole(TypographyRole::kLabel, t);
      const int32_t lw = static_cast<int32_t>(letters.measureTextWidth(leadingText_.c_str()));
      const int32_t lh = static_cast<int32_t>(letters.getHeight(1));
      letters.drawText(target, static_cast<int16_t>(cx - lw / 2), static_cast<int16_t>(cy - lh / 2),
                       leadingText_.c_str(), t.colors.onPrimaryContainer, t.colors.primaryContainer, false);
      textX = pill.x + pad + avatarSize + pad;
    } else if (icon_ != nullptr) {
      const Bounds iconRect(pill.x + pad, pill.centerY() - iconSize / 2, iconSize, iconSize);
      icon_(target, iconRect, foreground, /*thickness=*/2);
      textX = iconRect.right() + pad;
    }

    // ---- trailing slot (drawn first so the headline can stop before it)
    int32_t textRight = pill.right() - pad;
    if (!trailingText_.empty()) {
      tinygpu::IFont<RGB_T>& small = *fontForTypographyRole(TypographyRole::kLabel, t);
      const int32_t w = static_cast<int32_t>(small.measureTextWidth(trailingText_.c_str()));
      const int32_t h = static_cast<int32_t>(small.getHeight(1));
      small.drawText(target, static_cast<int16_t>(textRight - w), static_cast<int16_t>(pill.centerY() - h / 2),
                     trailingText_.c_str(), foreground, background, false);
      textRight -= w + pad;
    }
    if (trailingIcon_ != nullptr) {
      const Bounds iconRect(textRight - iconSize, pill.centerY() - iconSize / 2, iconSize, iconSize);
      trailingIcon_(target, iconRect, foreground, /*thickness=*/2);
      textRight = iconRect.x - pad;
    }

    // ---- headline and supporting text, centred as one block, clipped to the space left
    target.pushClipRect(toPx(textX), toPx(pill.y), toPx(textRight - textX), toPx(pill.h));
    tinygpu::IFont<RGB_T>& font = titleFont_ != nullptr ? *titleFont_ : *fontForTypographyRole(role_, t);
    const int32_t titleH = static_cast<int32_t>(font.getHeight(1));
    const int32_t textWidth = textRight - textX;
    if (supporting_.empty()) {
      const int32_t textY = pill.centerY() - titleH / 2;
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), title_.c_str(),
                    titleColor(textColor), background, false);
    } else {
      tinygpu::IFont<RGB_T>& small = subtitleFont_ != nullptr ? *subtitleFont_ : *fontForTypographyRole(TypographyRole::kLabel, t);
      const int32_t smallH = static_cast<int32_t>(small.getHeight(1));
      const int32_t gap = 2;
      const int32_t top = pill.centerY() - (titleH + gap + smallH) / 2;
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(top), title_.c_str(), titleColor(textColor),
                    background, false);
      (void)textWidth;  // long supporting text is clipped by the list
      small.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(top + titleH + gap),
                     supporting_.c_str(), foreground, background, false);
    }

    target.popClipRect();

    // ---- divider: full width, or inset from the text
    if (divider_) {
      const int32_t y = this->bounds.y + this->bounds.h - 1;
      const int32_t x0 = dividerInset_ ? textX : this->bounds.x;
      const int32_t x1 = this->bounds.x + this->bounds.w - (dividerInset_ ? pad : 0);
      target.drawLine(static_cast<size_t>(x0), static_cast<size_t>(y), static_cast<size_t>(x1),
                      static_cast<size_t>(y), outline);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (onClick) onClick();
    return true;
  }

 private:
  std::string title_;
  std::string supporting_;
  std::string leadingText_;
  std::string trailingText_;
  IconPainter<RGB_T> icon_ = nullptr;
  IconPainter<RGB_T> trailingIcon_ = nullptr;
  bool selected_ = false;
  bool divider_ = false;
  bool dividerInset_ = true;
  RGB_T dividerColor_{};
  bool hasDividerColor_ = false;
  RGB_T backgroundColor_{};
  bool hasBackgroundColor_ = false;
  TypographyRole role_ = TypographyRole::kBody;
  tinygpu::IFont<RGB_T>* titleFont_ = nullptr;
  tinygpu::IFont<RGB_T>* subtitleFont_ = nullptr;
  RGB_T titleColor_{};
  bool hasTitleColor_ = false;

  RGB_T titleColor(RGB_T textColor) const { return hasTitleColor_ ? titleColor_ : textColor; }
};

using ListItemRGB565 = ListItem<tinygpu::RGB565>;
using ListItemRGB666 = ListItem<tinygpu::RGB666>;
using ListItemRGB888 = ListItem<tinygpu::RGB888>;

}  // namespace tinymd
