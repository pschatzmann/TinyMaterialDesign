#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

enum class TypographyRole { kHeadline, kTitle, kBody, kLabel };
enum class TextAlign { kStart, kCenter };

/// Maps a TypographyRole to the theme's corresponding font - shared by
/// every widget that lets a caller pick a text size (Label, ListItem, ...)
/// rather than each duplicating this switch.
template <typename RGB_T>
tinygpu::IFont<RGB_T>* fontForTypographyRole(TypographyRole role, const MaterialTheme<RGB_T>& theme) {
  switch (role) {
    case TypographyRole::kHeadline:
      return theme.typography.headline;
    case TypographyRole::kTitle:
      return theme.typography.title;
    case TypographyRole::kLabel:
      return theme.typography.label;
    case TypographyRole::kBody:
    default:
      return theme.typography.body;
  }
}

/// Single line of themed, non-interactive text.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Label : public Widget<RGB_T> {
 public:
  Label() = default;
  Label(Bounds bounds, const char* text, TypographyRole role = TypographyRole::kBody,
       TextAlign align = TextAlign::kStart)
      : text_(text), role_(role), align_(align) {
    this->bounds = bounds;
  }

  void setText(const char* text) { text_ = text; }
  const std::string& text() const { return text_; }
  void setRole(TypographyRole role) { role_ = role; }
  void setAlign(TextAlign align) { align_ = align; }
  void setColor(RGB_T color) {
    colorOverride_ = color;
    hasColorOverride_ = true;
  }

  /// Explicit font, taking priority over the role-derived one in draw()
  /// below - nullptr (the default) means "use the theme's font for role()".
  void setFont(tinygpu::IFont<RGB_T>* font) { font_ = font; }
  tinygpu::IFont<RGB_T>* font() const { return font_; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    tinygpu::IFont<RGB_T>& font = font_ != nullptr ? *font_ : *fontForTypographyRole(role_, theme);
    RGB_T color = hasColorOverride_ ? colorOverride_ : theme.colors.onSurface;
    if (!this->enabled) color = blend(color, theme.colors.surface, 0.5f);

    const size_t textWidth = font.measureTextWidth(text_.c_str());
    const size_t textHeight = font.getHeight(1);
    const int32_t textX = align_ == TextAlign::kCenter
                              ? this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2
                              : this->bounds.x;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(textHeight) / 2;

    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 text_.c_str(), color, theme.colors.surface, false);
  }

 private:
  std::string text_;
  TypographyRole role_ = TypographyRole::kBody;
  TextAlign align_ = TextAlign::kStart;
  RGB_T colorOverride_{};
  bool hasColorOverride_ = false;
  tinygpu::IFont<RGB_T>* font_ = nullptr;
};

using LabelRGB565 = Label<tinygpu::RGB565>;
using LabelRGB666 = Label<tinygpu::RGB666>;
using LabelRGB888 = Label<tinygpu::RGB888>;

}  // namespace tinymd
