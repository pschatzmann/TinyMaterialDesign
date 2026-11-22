#pragma once
#include <functional>
#include <string>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/TextInputTarget.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Multi-line, word-wrapped text input box with scrolling.
 *
 * TextArea is TextField's multi-line counterpart - same focus/keyboard
 * wiring, same TextInputTarget contract (see Core/TextInputTarget.h and
 * TextField.h's class comment for the manage()/Keyboard pairing). Note that
 * a Keyboard's dedicated Enter key (not Done) is what inserts a newline
 * here - Done always finishes editing and closes the keyboard, exactly like
 * TextField, via the same handleEnter()/submit() pair.
 *
 * The text is wrapped at word boundaries to the width of the box. When the
 * text is taller than the box it scrolls: a drag over the box moves it, and
 * while typing the end of the text is kept in view. The text is drawn clipped
 * to the box.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class TextArea : public Widget<RGB_T>, public TextInputTarget {
 public:
  TextArea() = default;
  TextArea(Bounds bounds, const char* label = nullptr, const char* placeholder = nullptr) {
    this->bounds = bounds;
    if (label != nullptr) {
      label_ = label;
      hasLabel_ = true;
    }
    if (placeholder != nullptr) placeholder_ = placeholder;
  }

  /// Fired when the field becomes focused - a Keyboard's manage() wires
  /// this to attach itself, exactly like TextField.
  std::function<void()> onFocus;
  /// Fired after every character insertion/deletion.
  std::function<void(const std::string&)> onChange;
  /// Fired when a Keyboard's Done key (or submit()) is pressed.
  std::function<void()> onSubmit;

  const std::string& text() const { return text_; }
  void setText(const char* text) {
    text_ = text != nullptr ? text : "";
    scrollY_ = 0;
    if (onChange) onChange(text_);
  }
  /// 0 (the default) means unlimited.
  void setMaxLength(size_t maxLength) { maxLength_ = maxLength; }

  /// Colours of the entered text and of the placeholder (default: the theme's
  /// onSurface and onSurfaceVariant), as for TextField::setColors().
  void setColors(RGB_T text, RGB_T placeholder) {
    textColor_ = text;
    placeholderColor_ = placeholder;
    hasColors_ = true;
  }

  /// Vertical position of the text in pixels (0 = the first line)
  int32_t scrollOffset() const { return scrollY_; }

  bool focused() const { return focused_; }
  void focus() {
    focused_ = true;
    if (onFocus) onFocus();
  }
  void blur() override { focused_ = false; }

  void insertChar(char c) override {
    if (maxLength_ != 0 && text_.size() >= maxLength_) return;
    text_.push_back(c);
    if (onChange) onChange(text_);
  }
  void backspace() override {
    if (text_.empty()) return;
    text_.pop_back();
    if (onChange) onChange(text_);
  }
  /// Loses focus and fires onSubmit - called directly, or via a Keyboard's
  /// Done key through handleEnter(). Inserting a newline is the dedicated
  /// Enter key's job (Keyboard calls insertChar('\n') for that directly,
  /// bypassing this), so - like TextField - Done always finishes editing.
  void submit() {
    focused_ = false;
    if (onSubmit) onSubmit();
  }

  /// TextInputTarget: closes the keyboard, same as TextField - see submit().
  bool handleEnter() override {
    submit();
    return true;
  }

  /// A drag over the box scrolls the text, so the box is draggable.
  bool isDraggable() const override { return true; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const int32_t labelHeight = hasLabel_ ? 14 : 0;
    const Bounds box(this->bounds.x, this->bounds.y + labelHeight, this->bounds.w,
                     this->bounds.h - labelHeight);

    if (hasLabel_) {
      RGB_T labelColor = focused_ ? this->theme().colors.primary : this->theme().colors.onSurfaceVariant;
      if (!this->enabled) labelColor = blend(labelColor, this->theme().colors.surface, 0.5f);
      this->theme().typography.label->drawText(target, static_cast<int16_t>(this->bounds.x),
                                       static_cast<int16_t>(this->bounds.y), label_.c_str(),
                                       labelColor, this->theme().colors.surface, false);
    }

    RGB_T borderColor = focused_ ? this->theme().colors.primary : this->theme().colors.outline;
    if (!this->enabled) borderColor = blend(borderColor, this->theme().colors.surface, 0.5f);
    const size_t radius = toPx(this->theme().shape.small);
    target.fillRoundRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h), radius,
                         this->theme().colors.surface);
    target.drawRoundRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h), radius, borderColor);

    const int32_t pad = 8;
    const bool showPlaceholder = text_.empty() && !focused_ && !placeholder_.empty();
    const std::string& shown = showPlaceholder ? placeholder_ : text_;

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.body;
    const int32_t lineH = static_cast<int32_t>(font.getHeight(1));
    const int32_t viewW = box.w - 2 * pad;
    const int32_t viewH = box.h - 2 * pad;
    lines_ = wrap(shown, viewW, font);
    if (lines_.empty()) lines_.emplace_back();

    // the scroll range; while typing the end of the text stays in view
    const int32_t contentH = static_cast<int32_t>(lines_.size()) * lineH;
    maxScroll_ = contentH > viewH ? contentH - viewH : 0;
    if (focused_ && !showPlaceholder && contentH - scrollY_ > viewH) scrollY_ = contentH - viewH;
    clampScroll();

    const RGB_T textColor = showPlaceholder ? (hasColors_ ? placeholderColor_ : this->theme().colors.onSurfaceVariant)
                                            : (hasColors_ ? textColor_ : this->theme().colors.onSurface);
    target.pushClipRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h));
    for (size_t i = 0; i < lines_.size(); i++) {
      const int32_t y = box.y + pad + static_cast<int32_t>(i) * lineH - scrollY_;
      font.drawText(target, static_cast<int16_t>(box.x + pad), static_cast<int16_t>(y), lines_[i].c_str(),
                    textColor, this->theme().colors.surface, false);
    }
    target.popClipRect();

    // the caret after the last character
    if (focused_ && cursorVisible_ && !showPlaceholder) {
      const int32_t caretY = box.y + pad + static_cast<int32_t>(lines_.size() - 1) * lineH - scrollY_;
      const int32_t caretX = box.x + pad + static_cast<int32_t>(font.measureTextWidth(lines_.back().c_str()));
      if (caretY >= box.y && caretY + lineH <= box.bottom()) {
        target.drawLine(static_cast<size_t>(caretX + 1), static_cast<size_t>(caretY),
                        static_cast<size_t>(caretX + 1), static_cast<size_t>(caretY + lineH),
                        this->theme().colors.primary);
      }
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (event.type == tinygpu::GestureType::kPan || event.type == tinygpu::GestureType::kScroll) {
      // the text follows the finger: a drag up moves the text up
      if (event.phase == tinygpu::GesturePhase::kChanged) {
        scrollY_ -= event.stepDeltaY;
        clampScroll();
      }
      return true;
    }
    if (!isTapGesture(event.type)) return false;
    focus();
    return true;
  }

  bool update(uint32_t nowMs) override {
    const bool wasVisible = cursorVisible_;
    if (!focused_) {
      cursorVisible_ = false;
      return wasVisible != cursorVisible_;
    }
    if (nowMs - lastBlinkMs_ >= kBlinkIntervalMs) {
      cursorVisible_ = !cursorVisible_;
      lastBlinkMs_ = nowMs;
    }
    return wasVisible != cursorVisible_;
  }

 private:
  static constexpr uint32_t kBlinkIntervalMs = 500;

  std::string text_;
  std::string label_;
  std::string placeholder_;
  bool hasLabel_ = false;
  size_t maxLength_ = 0;
  bool focused_ = false;
  bool cursorVisible_ = true;
  uint32_t lastBlinkMs_ = 0;
  bool hasColors_ = false;
  RGB_T textColor_{};
  RGB_T placeholderColor_{};

  // scrolling: the wrapped lines of the last draw, and how far they are scrolled
  std::vector<std::string> lines_;
  int32_t scrollY_ = 0;
  int32_t maxScroll_ = 0;

  void clampScroll() {
    if (scrollY_ < 0) scrollY_ = 0;
    if (scrollY_ > maxScroll_) scrollY_ = maxScroll_;
  }

  /// Breaks the text into lines no wider than maxW: at spaces, and at newlines.
  /// A word wider than a line is split between characters.
  static std::vector<std::string> wrap(const std::string& text, int32_t maxW,
                                       tinygpu::IFont<RGB_T>& font) {
    auto fits = [&](const std::string& s) {
      return static_cast<int32_t>(font.measureTextWidth(s.c_str())) <= maxW;
    };
    std::vector<std::string> out;
    std::string current;
    std::string word;
    for (size_t i = 0; i <= text.size(); i++) {
      const char c = i < text.size() ? text[i] : '\n';
      if (c != ' ' && c != '\n') {
        word += c;
        continue;
      }
      if (!word.empty()) {
        const std::string sep = current.empty() ? "" : " ";
        if (fits(current + sep + word)) {
          current += sep + word;
        } else {
          if (!current.empty()) {
            out.push_back(current);
            current.clear();
          }
          current = word;
          while (!fits(current) && current.size() > 1) {
            size_t k = 1;
            while (k < current.size() && fits(current.substr(0, k + 1))) k++;
            out.push_back(current.substr(0, k));
            current = current.substr(k);
          }
        }
        word.clear();
      }
      if (c == '\n') {
        out.push_back(current);
        current.clear();
      }
    }
    if (!current.empty()) out.push_back(current);
    return out;
  }
};

using TextAreaRGB565 = TextArea<tinygpu::RGB565>;
using TextAreaRGB666 = TextArea<tinygpu::RGB666>;
using TextAreaRGB888 = TextArea<tinygpu::RGB888>;

}  // namespace tinymd
