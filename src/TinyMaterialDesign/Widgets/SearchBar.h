#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/TextInputTarget.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Draw/Icons.h"

namespace tinymd {

/**
 * @brief Pill-shaped search input: leading magnifier glyph, typed text, and
 * a trailing "x" clear glyph shown once there's something to clear.
 *
 * Implements TextInputTarget the same way TextField does, so it can be
 * driven by a Keyboard via Keyboard::manage() - see TextField.h's class
 * comment for the field/keyboard wiring example, which applies here
 * unchanged. Enter submits, same single-line semantics as TextField.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class SearchBar : public Widget<RGB_T>, public TextInputTarget {
 public:
  SearchBar() = default;
  SearchBar(Bounds bounds, const char* placeholder = nullptr) {
    this->bounds = bounds;
    if (placeholder != nullptr) placeholder_ = placeholder;
  }

  std::function<void()> onFocus;
  std::function<void(const std::string&)> onChange;
  std::function<void()> onSubmit;

  const std::string& text() const { return text_; }
  void setText(const char* text) {
    text_ = text != nullptr ? text : "";
    if (onChange) onChange(text_);
  }

  bool focused() const { return focused_; }
  void focus() {
    focused_ = true;
    if (onFocus) onFocus();
  }
  void blur() override { focused_ = false; }

  void insertChar(char c) override {
    text_.push_back(c);
    if (onChange) onChange(text_);
  }
  void backspace() override {
    if (text_.empty()) return;
    text_.pop_back();
    if (onChange) onChange(text_);
  }
  void submit() {
    focused_ = false;
    if (onSubmit) onSubmit();
  }
  bool handleEnter() override {
    submit();
    return true;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T borderColor = focused_ ? this->theme().colors.primary : this->theme().colors.outline;
    if (!this->enabled) borderColor = blend(borderColor, this->theme().colors.surface, 0.5f);
    const size_t radius = toPx(this->bounds.h / 2);

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.surfaceVariant);
    if (focused_) {
      target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                           toPx(this->bounds.h), radius, borderColor);
    }

    const int32_t iconSize = this->bounds.h - 16;
    const RGB_T iconColor = this->theme().colors.onSurfaceVariant;
    const Bounds searchIconRect(this->bounds.x + 12, this->bounds.centerY() - iconSize / 2, iconSize,
                                iconSize);
    drawSearch(target, searchIconRect, iconColor, /*thickness=*/2);

    const int32_t textX = searchIconRect.right() + 8;
    const int32_t clearIconWidth = !text_.empty() ? iconSize + 12 : 0;
    const int32_t textRight = this->bounds.right() - 12 - clearIconWidth;

    tinygpu::IFont<RGB_T>& font = *this->theme().typography.body;
    const int32_t textY = this->bounds.centerY() - static_cast<int32_t>(font.getHeight(1)) / 2;

    if (text_.empty()) {
      if (!focused_ && !placeholder_.empty()) {
        font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                     placeholder_.c_str(), this->theme().colors.onSurfaceVariant,
                     this->theme().colors.surfaceVariant, false);
      }
    } else {
      font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY), text_.c_str(),
                   this->theme().colors.onSurface, this->theme().colors.surfaceVariant, false);
      clearIconRect_ = Bounds(textRight, this->bounds.centerY() - iconSize / 2, iconSize, iconSize);
      drawClose(target, clearIconRect_, iconColor, /*thickness=*/2);
    }

    if (focused_ && cursorVisible_) {
      const size_t textWidth = font.measureTextWidth(text_.c_str());
      const int32_t cursorX = textX + static_cast<int32_t>(textWidth) + 1;
      target.drawLine(toPx(cursorX), toPx(this->bounds.y + 6), toPx(cursorX),
                      toPx(this->bounds.bottom() - 6), this->theme().colors.primary);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (!text_.empty() && clearIconRect_.contains(event.point.x, event.point.y)) {
      setText("");
      return true;
    }
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
  std::string placeholder_;
  bool focused_ = false;
  bool cursorVisible_ = true;
  uint32_t lastBlinkMs_ = 0;
  Bounds clearIconRect_;
};

using SearchBarRGB565 = SearchBar<tinygpu::RGB565>;
using SearchBarRGB666 = SearchBar<tinygpu::RGB666>;
using SearchBarRGB888 = SearchBar<tinygpu::RGB888>;

}  // namespace tinymd
