#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyGPU/Font/LinePrinter.h"
#include "TinyMaterialDesign/Core/TextInputTarget.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Multi-line, word-wrapped text input box.
 *
 * TextArea is TextField's multi-line counterpart - same focus/keyboard
 * wiring, same TextInputTarget contract (see Core/TextInputTarget.h and
 * TextField.h's class comment for the manage()/Keyboard pairing). Note that
 * a Keyboard's dedicated Enter key (not Done) is what inserts a newline
 * here - Done always finishes editing and closes the keyboard, exactly like
 * TextField, via the same handleEnter()/submit() pair.
 *
 * Wrapping reuses TinyGPU's existing LinePrinter (the same one Card and
 * Dialog use for their body text) rather than reimplementing word wrap. The
 * blinking end-of-text cursor uses LinePrinter::cursorX()/cursorY(), which
 * report where the next character would land after a print() call.
 *
 * Known limitation: no scrolling - text past the box's bottom edge is
 * cropped to the box (see ISurface::pushClipRect()) rather than scrolling
 * into view, and the cursor is skipped once it would fall past the box for
 * the same reason. Fine for a few lines of notes; not a substitute for a
 * real multi-page, scrollable text editor.
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

    // Defaults for an empty field - overwritten below once the printer
    // actually runs, so the cursor still has somewhere sensible to sit.
    size_t cursorX = toPx(box.x + pad);
    size_t cursorY = toPx(box.y + pad);

    if (!shown.empty()) {
      const int32_t rightEdge = box.right() - pad;
      const size_t rightBorder = rightEdge < static_cast<int32_t>(target.width())
                                    ? target.width() - static_cast<size_t>(rightEdge)
                                    : 0;

      // Crops text past the box's bottom edge to the box (see the class
      // comment) instead of letting it draw past it - LinePrinter/drawText
      // have no notion of "this widget's own box", only the whole surface,
      // so without this a long enough value would spill below the box.
      target.pushClipRect(toPx(box.x), toPx(box.y), toPx(box.w), toPx(box.h));
      tinygpu::LinePrinter<RGB_T> printer;
      printer.setFont(*this->theme().typography.body);
      printer.setTarget(target);
      printer.setColor(showPlaceholder ? this->theme().colors.onSurfaceVariant : this->theme().colors.onSurface);
      printer.setTopBorder(toPx(box.y + pad));
      printer.setLeftBorder(toPx(box.x + pad));
      printer.setRightBorder(rightBorder);
      printer.print(shown.c_str());
      cursorX = printer.cursorX();
      cursorY = printer.cursorY();
      target.popClipRect();
    }

    if (focused_ && cursorVisible_ && !showPlaceholder) {
      const size_t lineHeight = this->theme().typography.body->getHeight(1);
      if (cursorY + lineHeight <= toPx(box.bottom())) {
        target.drawLine(cursorX + 1, cursorY, cursorX + 1, cursorY + lineHeight,
                        this->theme().colors.primary);
      }
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
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
};

using TextAreaRGB565 = TextArea<tinygpu::RGB565>;
using TextAreaRGB666 = TextArea<tinygpu::RGB666>;
using TextAreaRGB888 = TextArea<tinygpu::RGB888>;

}  // namespace tinymd
