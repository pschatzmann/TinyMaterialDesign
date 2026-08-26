#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/TextInputTarget.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Single-line text input box.
 *
 * TextField only holds and displays text and focus state - it has no idea
 * how to turn touches into characters. Pair it with a Keyboard (Keyboard.h):
 *
 *   Keyboard<RGB565> keyboard(Bounds(0, 420, 340, 190));
 *   TextField<RGB565> nameField(Bounds(16, 88, 220, 48), "Name");
 *   keyboard.manage(nameField);   // wires nameField's focus to this keyboard
 *   screen.addWidget(nameField);
 *   screen.addWidget(keyboard);   // add last so it draws on top when shown
 *
 * insertChar()/backspace()/handleEnter() (TextInputTarget, see Core/
 * TextInputTarget.h) are what let a Keyboard - or any other input source -
 * drive a focused field without TextField needing to know about it. Enter
 * submits (see TextArea.h for the multi-line counterpart, where Enter
 * inserts a newline instead).
 *
 * Bounds is split internally into a small label strip (top) and the input
 * box itself (the rest) - both are part of this widget's own bounds, so a
 * tap anywhere in either area focuses the field.
 *
 * Known limitation: text longer than the box is drawn past its right edge
 * rather than scrolling/clipping - TinyGPU's fonts have no sub-rect clip,
 * only surface-edge clipping. Fine for short inputs (names, search terms,
 * ...); not meant for long free-form text.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class TextField : public Widget<RGB_T>, public TextInputTarget {
 public:
  TextField() = default;
  TextField(Bounds bounds, const char* label = nullptr, const char* placeholder = nullptr) {
    this->bounds = bounds;
    if (label != nullptr) {
      label_ = label;
      hasLabel_ = true;
    }
    if (placeholder != nullptr) placeholder_ = placeholder;
  }

  /// Fired when the field becomes focused (by tap, or by any other code
  /// calling focus()) - a Keyboard's manage() wires this to attach itself.
  std::function<void()> onFocus;
  /// Fired after every character insertion/deletion.
  std::function<void(const std::string&)> onChange;
  /// Fired when a Keyboard's Enter/Done key (or submit()) is pressed.
  std::function<void()> onSubmit;

  const std::string& text() const { return text_; }
  void setText(const char* text) {
    text_ = text != nullptr ? text : "";
    if (onChange) onChange(text_);
  }
  /// 0 (the default) means unlimited.
  void setMaxLength(size_t maxLength) { maxLength_ = maxLength; }

  bool focused() const { return focused_; }
  void focus() {
    focused_ = true;
    if (onFocus) onFocus();
  }
  void blur() override { focused_ = false; }

  /// Appends one character - called by whatever is driving text entry
  /// (typically a Keyboard key tap).
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
  /// Enter/Done key through handleEnter().
  void submit() {
    focused_ = false;
    if (onSubmit) onSubmit();
  }

  /// TextInputTarget: single-line semantics - Enter submits and tells the
  /// Keyboard to hide itself.
  bool handleEnter() override {
    submit();
    return true;
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    const int32_t labelHeight = hasLabel_ ? 14 : 0;
    const Bounds box(this->bounds.x, this->bounds.y + labelHeight, this->bounds.w,
                     this->bounds.h - labelHeight);

    if (hasLabel_) {
      RGB_T labelColor = focused_ ? theme.colors.primary : theme.colors.onSurfaceVariant;
      if (!this->enabled) labelColor = blend(labelColor, theme.colors.surface, 0.5f);
      theme.typography.label->drawText(target, static_cast<int16_t>(this->bounds.x),
                                       static_cast<int16_t>(this->bounds.y), label_.c_str(),
                                       labelColor, theme.colors.surface, false);
    }

    RGB_T borderColor = focused_ ? theme.colors.primary : theme.colors.outline;
    if (!this->enabled) borderColor = blend(borderColor, theme.colors.surface, 0.5f);
    const size_t radius = toPx(theme.shape.small);
    target.fillRoundRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h), radius,
                         theme.colors.surface);
    target.drawRoundRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h), radius, borderColor);

    const int32_t pad = 8;
    const int32_t textX = box.x + pad;
    const int32_t textY = box.centerY() - static_cast<int32_t>(theme.typography.body->getHeight(1)) / 2;

    if (text_.empty() && !focused_) {
      if (!placeholder_.empty()) {
        theme.typography.body->drawText(target, static_cast<int16_t>(textX),
                                        static_cast<int16_t>(textY), placeholder_.c_str(),
                                        theme.colors.onSurfaceVariant, theme.colors.surface, false);
      }
      return;
    }

    theme.typography.body->drawText(target, static_cast<int16_t>(textX),
                                    static_cast<int16_t>(textY), text_.c_str(),
                                    theme.colors.onSurface, theme.colors.surface, false);

    if (focused_ && cursorVisible_) {
      const size_t textWidth = theme.typography.body->measureTextWidth(text_.c_str());
      const int32_t cursorX = textX + static_cast<int32_t>(textWidth) + 1;
      target.drawLine(toPx(cursorX), toPx(box.y + 6), toPx(cursorX), toPx(box.bottom() - 6),
                      theme.colors.primary);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    focus();
    return true;
  }

  void update(uint32_t nowMs) override {
    if (!focused_) {
      cursorVisible_ = false;
      return;
    }
    if (nowMs - lastBlinkMs_ >= kBlinkIntervalMs) {
      cursorVisible_ = !cursorVisible_;
      lastBlinkMs_ = nowMs;
    }
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
};

using TextFieldRGB565 = TextField<tinygpu::RGB565>;
using TextFieldRGB666 = TextField<tinygpu::RGB666>;
using TextFieldRGB888 = TextField<tinygpu::RGB888>;

}  // namespace tinymd
