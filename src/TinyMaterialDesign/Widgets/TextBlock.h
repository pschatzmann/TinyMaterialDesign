#pragma once
#include <string>

#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/Label.h"

namespace tinymd {

/**
 * @brief Multi-line text that is wrapped at word boundaries to the widget
 * width ('\n' starts a new line) - for notes, explanations and messages.
 * Unlike Label it can span several lines; heightFor() tells how much room
 * the text needs, so a layout can size the widget to its text:
 *
 *   TextBlock<RGB565> note(Bounds(8, y, 224, 0), "A long explanation ...");
 *   page.addChild(note);  // gives it the theme
 *   note.bounds.h = note.heightFor(note.bounds.w);
 *
 * Default: label font, onSurfaceVariant color, drawn on the page background.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class TextBlock : public Widget<RGB_T> {
 public:
  TextBlock() = default;
  explicit TextBlock(const char* text, TypographyRole role = TypographyRole::kLabel)
      : text_(text != nullptr ? text : ""), role_(role) {}
  TextBlock(Bounds bounds, const char* text, TypographyRole role = TypographyRole::kLabel)
      : text_(text != nullptr ? text : ""), role_(role) {
    this->bounds = bounds;
  }

  void setText(const char* text) { text_ = text != nullptr ? text : ""; }
  const std::string& text() const { return text_; }
  void setRole(TypographyRole role) { role_ = role; }
  void setColor(RGB_T color) {
    color_ = color;
    hasColor_ = true;
  }
  void clearColor() { hasColor_ = false; }
  /// Draw on a surface (e.g. inside a Card) instead of the page background.
  void setOnSurface(bool onSurface) { onSurface_ = onSurface; }
  /// Extra pixels between the lines.
  void setLineSpacing(int32_t pixels) { lineSpacing_ = pixels; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    tinygpu::IFont<RGB_T>& font = *fontForTypographyRole(role_, t);
    const RGB_T background = onSurface_ ? t.colors.surface : t.colors.background;
    RGB_T color = hasColor_ ? color_ : t.colors.onSurfaceVariant;
    if (!this->enabled) color = blend(color, background, 0.5f);
    int32_t y = this->bounds.y;
    const int32_t lineHeight = static_cast<int32_t>(font.getHeight(1)) + lineSpacing_;
    forEachLine(text_, this->bounds.w, font, [&](const std::string& line) {
      font.drawText(target, static_cast<int16_t>(this->bounds.x), static_cast<int16_t>(y), line.c_str(), color,
                    background, false);
      y += lineHeight;
    });
  }

  /// Height that the text needs at the given width (needs the theme).
  int32_t heightFor(int32_t width) const {
    tinygpu::IFont<RGB_T>& font = *fontForTypographyRole(role_, this->theme());
    return lineCount(text_, width, font) * (static_cast<int32_t>(font.getHeight(1)) + lineSpacing_);
  }

  /// Number of lines of `text` wrapped to `width` with `font`.
  static int32_t lineCount(const std::string& text, int32_t width, tinygpu::IFont<RGB_T>& font) {
    int32_t lines = 0;
    forEachLine(text, width, font, [&](const std::string&) { lines++; });
    return lines;
  }

  /// Calls fn(line) for each line of `text` wrapped to `width`.
  template <typename Fn>
  static void forEachLine(const std::string& text, int32_t width, tinygpu::IFont<RGB_T>& font, Fn fn) {
    std::string line, word;
    auto fits = [&](const std::string& s) { return static_cast<int32_t>(font.measureTextWidth(s.c_str())) <= width; };
    for (size_t i = 0; i <= text.size(); ++i) {
      const char c = i < text.size() ? text[i] : ' ';
      if (c != ' ' && c != '\n') {
        word += c;
        continue;
      }
      if (!word.empty()) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && !fits(candidate)) {
          fn(line);
          line = word;
        } else {
          line = candidate;
        }
        word.clear();
      }
      if (c == '\n') {
        fn(line);  // also empty lines
        line.clear();
      }
    }
    if (!line.empty()) fn(line);
  }

 private:
  std::string text_;
  TypographyRole role_ = TypographyRole::kLabel;
  RGB_T color_{};
  bool hasColor_ = false;
  bool onSurface_ = false;
  int32_t lineSpacing_ = 3;
};

}  // namespace tinymd
