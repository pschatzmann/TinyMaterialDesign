#pragma once
#include <cctype>
#include <functional>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/TextInputTarget.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief On-screen QWERTY keyboard that drives one text input at a time.
 *
 * A single Keyboard instance can serve any number of fields, TextField and
 * TextArea interchangeably (see Core/TextInputTarget.h) - call manage(field)
 * once per field to wire it up:
 *
 *   Keyboard<RGB565> keyboard(Bounds(0, 420, 340, 190));
 *   keyboard.manage(nameField);    // TextField
 *   keyboard.manage(notesField);   // TextArea
 *
 * manage() makes tapping a field show this keyboard targeted at it (and
 * blur whichever field it was previously targeting). Add the keyboard to
 * Screen *after* every field it manages, so it draws (and hit-tests) on top
 * of them while shown.
 *
 * Layout: a numbers row (which also holds the "123"/"ABC" key), three rows
 * that switch between QWERTY letters and a page of common symbols
 * (!@#$%^&*() / -_=+[]{}\\ / ;:'",./), and a control row (Shift / Space /
 * Enter / Backspace / Done). Shift is a toggle (caps-lock style, not "one
 * letter then auto-revert") and has no effect on the symbols page. Enter and
 * Done are deliberately separate keys: Enter always inserts a literal '\n'
 * and moves to the next line, regardless of field type; Done always means
 * "finished editing" and closes the keyboard (see TextInputTarget::
 * handleEnter(), which every field implements to return true) - so on a
 * TextArea, Enter adds a line without leaving the keyboard, and Done closes
 * it whenever you're actually done, mid-line or not. There is only this one
 * symbols page (no further "more symbols" page) - a deliberate cut to keep
 * this a two-layer keyboard rather than a deeper multi-page one.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Keyboard : public Widget<RGB_T> {
 public:
  Keyboard() = default;
  explicit Keyboard(Bounds bounds) {
    this->bounds = bounds;
    this->visible = false;
  }

  /// Wires `field` so tapping it shows this keyboard targeted at it. `Field`
  /// must implement TextInputTarget and expose a public
  /// `std::function<void()> onFocus` - true of both TextField and TextArea.
  template <typename Field>
  void manage(Field& field) {
    field.onFocus = [this, &field]() {
      if (target_ != nullptr && target_ != &field) target_->blur();
      target_ = &field;
      this->visible = true;
    };
  }

  void hide() { this->visible = false; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    target.fillRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                    toPx(this->bounds.h), theme.colors.surfaceVariant);

    forEachKey([&](const Key& key, const Bounds& rect) {
      const bool highlighted = key.action == Action::kShift && shift_;
      const RGB_T keyColor = highlighted ? theme.colors.secondaryContainer : theme.colors.surface;
      target.fillRoundRect(toPx(rect.x + 2), toPx(rect.y + 2), toPx(rect.w - 4), toPx(rect.h - 4),
                           toPx(theme.shape.small), keyColor);

      char labelBuffer[2] = {0, 0};
      const char* labelText = key.label;
      if (key.action == Action::kChar) {
        labelBuffer[0] = shift_ ? static_cast<char>(std::toupper(key.ch)) : key.ch;
        labelText = labelBuffer;
      } else if (key.action == Action::kToggleSymbols) {
        labelText = (mode_ == Mode::kLetters) ? "123" : "ABC";
      }

      const size_t textWidth = theme.typography.body->measureTextWidth(labelText);
      const int32_t textX = rect.centerX() - static_cast<int32_t>(textWidth) / 2;
      const int32_t textY = rect.centerY() - static_cast<int32_t>(theme.typography.body->getHeight(1)) / 2;
      theme.typography.body->drawText(target, static_cast<int16_t>(textX),
                                      static_cast<int16_t>(textY), labelText,
                                      theme.colors.onSurface, keyColor, false);
      return false;
    });
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    const Key* hit = nullptr;
    forEachKey([&](const Key& key, const Bounds& rect) {
      if (rect.contains(event.point.x, event.point.y)) {
        hit = &key;
        return true;  // stop iterating
      }
      return false;
    });
    if (hit == nullptr) return true;  // swallow the tap either way
    handleKey(*hit);
    return true;
  }

 private:
  enum class Action { kChar, kShift, kBackspace, kSpace, kNewline, kEnter, kToggleSymbols };
  enum class Mode { kLetters, kSymbols };

  struct Key {
    Action action;
    char ch;
    const char* label;
    float weight;
  };

  // Numbers row, plus a "123"/"ABC" key (prepended) that swaps rows 1-3
  // between letters and symbols - the numbers row itself stays the same in
  // both modes.
  static constexpr Key kRow0[] = {
      {Action::kToggleSymbols, 0, "123", 1.5f},
      {Action::kChar, '1', "1", 1}, {Action::kChar, '2', "2", 1}, {Action::kChar, '3', "3", 1},
      {Action::kChar, '4', "4", 1}, {Action::kChar, '5', "5", 1}, {Action::kChar, '6', "6", 1},
      {Action::kChar, '7', "7", 1}, {Action::kChar, '8', "8", 1}, {Action::kChar, '9', "9", 1},
      {Action::kChar, '0', "0", 1},
  };
  static constexpr Key kRow1[] = {
      {Action::kChar, 'q', "q", 1}, {Action::kChar, 'w', "w", 1}, {Action::kChar, 'e', "e", 1},
      {Action::kChar, 'r', "r", 1}, {Action::kChar, 't', "t", 1}, {Action::kChar, 'y', "y", 1},
      {Action::kChar, 'u', "u", 1}, {Action::kChar, 'i', "i", 1}, {Action::kChar, 'o', "o", 1},
      {Action::kChar, 'p', "p", 1},
  };
  static constexpr Key kRow2[] = {
      {Action::kChar, 'a', "a", 1}, {Action::kChar, 's', "s", 1}, {Action::kChar, 'd', "d", 1},
      {Action::kChar, 'f', "f", 1}, {Action::kChar, 'g', "g", 1}, {Action::kChar, 'h', "h", 1},
      {Action::kChar, 'j', "j", 1}, {Action::kChar, 'k', "k", 1}, {Action::kChar, 'l', "l", 1},
  };
  static constexpr Key kRow3[] = {
      {Action::kChar, 'z', "z", 1}, {Action::kChar, 'x', "x", 1}, {Action::kChar, 'c', "c", 1},
      {Action::kChar, 'v', "v", 1}, {Action::kChar, 'b', "b", 1}, {Action::kChar, 'n', "n", 1},
      {Action::kChar, 'm', "m", 1},
  };
  // Symbols page, same shape (10/9/7 keys) as the letter rows it replaces.
  static constexpr Key kRow1Symbols[] = {
      {Action::kChar, '!', "!", 1}, {Action::kChar, '@', "@", 1}, {Action::kChar, '#', "#", 1},
      {Action::kChar, '$', "$", 1}, {Action::kChar, '%', "%", 1}, {Action::kChar, '^', "^", 1},
      {Action::kChar, '&', "&", 1}, {Action::kChar, '*', "*", 1}, {Action::kChar, '(', "(", 1},
      {Action::kChar, ')', ")", 1},
  };
  static constexpr Key kRow2Symbols[] = {
      {Action::kChar, '-', "-", 1}, {Action::kChar, '_', "_", 1}, {Action::kChar, '=', "=", 1},
      {Action::kChar, '+', "+", 1}, {Action::kChar, '[', "[", 1}, {Action::kChar, ']', "]", 1},
      {Action::kChar, '{', "{", 1}, {Action::kChar, '}', "}", 1}, {Action::kChar, '\\', "\\", 1},
  };
  static constexpr Key kRow3Symbols[] = {
      {Action::kChar, ';', ";", 1}, {Action::kChar, ':', ":", 1}, {Action::kChar, '\'', "'", 1},
      {Action::kChar, '"', "\"", 1}, {Action::kChar, ',', ",", 1}, {Action::kChar, '.', ".", 1},
      {Action::kChar, '/', "/", 1},
  };
  static constexpr Key kRow4[] = {
      {Action::kShift, 0, "Shift", 1.5f},
      {Action::kSpace, 0, "", 3.5f},
      {Action::kNewline, 0, "Enter", 1.5f},
      {Action::kBackspace, 0, "<-", 1.5f},
      {Action::kEnter, 0, "Done", 1.5f},
  };

  TextInputTarget* target_ = nullptr;
  bool shift_ = false;
  Mode mode_ = Mode::kLetters;

  /// Walks every key, computing its Bounds from its row's weighted layout,
  /// invoking `visitor(key, rect)`. Used by both draw() and onGesture() so
  /// the layout math lives in exactly one place. `visitor` returns true to
  /// stop iterating early (hit-testing); draw() always returns false.
  /// Rows 1-3 swap between letters and symbols depending on mode_; row 0
  /// (numbers + the mode toggle) and row 4 (controls) are the same either way.
  template <typename Visitor>
  void forEachKey(Visitor visitor) const {
    struct Row {
      const Key* keys;
      size_t count;
    };
    const bool symbols = mode_ == Mode::kSymbols;
    const Row rows[] = {
        {kRow0, sizeof(kRow0) / sizeof(kRow0[0])},
        symbols ? Row{kRow1Symbols, sizeof(kRow1Symbols) / sizeof(kRow1Symbols[0])}
               : Row{kRow1, sizeof(kRow1) / sizeof(kRow1[0])},
        symbols ? Row{kRow2Symbols, sizeof(kRow2Symbols) / sizeof(kRow2Symbols[0])}
               : Row{kRow2, sizeof(kRow2) / sizeof(kRow2[0])},
        symbols ? Row{kRow3Symbols, sizeof(kRow3Symbols) / sizeof(kRow3Symbols[0])}
               : Row{kRow3, sizeof(kRow3) / sizeof(kRow3[0])},
        {kRow4, sizeof(kRow4) / sizeof(kRow4[0])},
    };
    constexpr size_t kRowCount = sizeof(rows) / sizeof(rows[0]);
    const int32_t rowHeight = this->bounds.h / static_cast<int32_t>(kRowCount);

    for (size_t r = 0; r < kRowCount; ++r) {
      float totalWeight = 0.0f;
      for (size_t i = 0; i < rows[r].count; ++i) totalWeight += rows[r].keys[i].weight;

      const int32_t rowY = this->bounds.y + static_cast<int32_t>(r) * rowHeight;
      float cursor = 0.0f;
      for (size_t i = 0; i < rows[r].count; ++i) {
        const Key& key = rows[r].keys[i];
        const int32_t x0 = this->bounds.x + static_cast<int32_t>(cursor / totalWeight * this->bounds.w);
        cursor += key.weight;
        const int32_t x1 = this->bounds.x + static_cast<int32_t>(cursor / totalWeight * this->bounds.w);
        const Bounds rect(x0, rowY, x1 - x0, rowHeight);
        if (visitor(key, rect)) return;
      }
    }
  }

  void handleKey(const Key& key) {
    switch (key.action) {
      case Action::kChar:
        if (target_ != nullptr) {
          target_->insertChar(shift_ ? static_cast<char>(std::toupper(key.ch)) : key.ch);
        }
        break;
      case Action::kShift:
        shift_ = !shift_;
        break;
      case Action::kBackspace:
        if (target_ != nullptr) target_->backspace();
        break;
      case Action::kSpace:
        if (target_ != nullptr) target_->insertChar(' ');
        break;
      case Action::kNewline:
        // Unlike Done/handleEnter(), this always inserts a literal '\n' and
        // moves to the next line, regardless of the target's field type.
        if (target_ != nullptr) target_->insertChar('\n');
        break;
      case Action::kEnter:
        // Both TextField and TextArea return true from handleEnter() -
        // Done always finishes editing and closes the keyboard.
        if (target_ == nullptr || target_->handleEnter()) this->visible = false;
        break;
      case Action::kToggleSymbols:
        mode_ = (mode_ == Mode::kLetters) ? Mode::kSymbols : Mode::kLetters;
        break;
    }
  }
};

using KeyboardRGB565 = Keyboard<tinygpu::RGB565>;
using KeyboardRGB666 = Keyboard<tinygpu::RGB666>;
using KeyboardRGB888 = Keyboard<tinygpu::RGB888>;

}  // namespace tinymd
