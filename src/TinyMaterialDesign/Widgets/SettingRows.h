#pragma once
#include <math.h>
#include <stdio.h>

#include <functional>
#include <string>

#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/RadioButton.h"
#include "TinyMaterialDesign/Widgets/Slider.h"
#include "TinyMaterialDesign/Widgets/Switch.h"

namespace tinymd {

/**
 * @file SettingRows.h
 * @brief Rows for settings and detail pages: a caption combined with a
 * control or a value. Each row is one widget, so a page only places rows
 * from top to bottom. By default they are drawn on the page background; call
 * setOnSurface(true) for rows inside a Card.
 *
 * - SliderRow: caption and formatted value, a Slider below (kHeight 40)
 * - SwitchRow: caption, Switch at the right; tapping the row toggles it
 * - RadioRow: RadioButton and caption; tapping the row selects it
 * - KeyValueRow: key at the left, value at the right
 */

/// Common part of the rows: caption and background.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class SettingRow : public Widget<RGB_T> {
 public:
  void setCaption(const char* caption) { caption_ = caption != nullptr ? caption : ""; }
  const std::string& caption() const { return caption_; }
  /// Draw on a surface (e.g. inside a Card) instead of the page background.
  void setOnSurface(bool onSurface) { onSurface_ = onSurface; }

 protected:
  std::string caption_;
  bool onSurface_ = false;

  RGB_T background() const { return onSurface_ ? this->theme().colors.surface : this->theme().colors.background; }

  void text(tinygpu::ISurface<RGB_T>& target, tinygpu::IFont<RGB_T>& font, int32_t x, int32_t y, const char* s,
            RGB_T color) {
    font.drawText(target, static_cast<int16_t>(x), static_cast<int16_t>(y), s, color, background(), false);
  }

  RGB_T dimmed(RGB_T color) const { return this->enabled ? color : blend(color, background(), 0.5f); }
};

/**
 * @brief Caption and formatted value on one line, a Slider below. The value
 * is quantized to `step`:
 *
 *   SliderRow<RGB565> gain("Gain", 0, 40, 1, [](float v, char* b, size_t n) {
 *     snprintf(b, n, "%+.0f dB", v);
 *   });
 *   gain.onChange = [](float v) { ... };
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class SliderRow : public SettingRow<RGB_T> {
 public:
  using Formatter = std::function<void(float value, char* buf, size_t len)>;
  static constexpr int32_t kHeight = 40;

  SliderRow() = default;
  SliderRow(const char* caption, float minValue, float maxValue, float step, Formatter format = nullptr)
      : step_(step), format_(std::move(format)), slider_(Bounds(), minValue, maxValue) {
    this->setCaption(caption);
    slider_.onChange = [this](float v) {
      const float q = quantize(v);
      if (q == value_) return;
      value_ = q;
      if (onChange) onChange(value_);
    };
  }

  std::function<void(float)> onChange;

  float value() const { return value_; }
  void setValue(float value) {
    value_ = quantize(value);
    slider_.setValue(value_);
  }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    slider_.setTheme(theme);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    tinygpu::IFont<RGB_T>& small = *t.typography.label;
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    this->text(target, small, b.x, b.y + 3, this->caption_.c_str(), this->dimmed(t.colors.onSurfaceVariant));
    char buf[24] = "";
    if (format_) {
      format_(value_, buf, sizeof(buf));
    } else {
      snprintf(buf, sizeof(buf), "%g", value_);
    }
    this->text(target, body, b.right() - static_cast<int32_t>(body.measureTextWidth(buf)), b.y + 2, buf,
               this->dimmed(t.colors.onSurface));
    syncSlider();
    slider_.enabled = this->enabled;
    slider_.draw(target);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    syncSlider();
    return slider_.onGesture(event);
  }

  bool isDraggableAt(int32_t x, int32_t y) const override {
    (void)x;
    return y >= this->bounds.y + kTextHeight;
  }

 private:
  static constexpr int32_t kTextHeight = 14;
  float step_ = 1.0f;
  float value_ = 0.0f;
  Formatter format_;
  Slider<RGB_T> slider_;

  float quantize(float v) const { return step_ > 0 ? roundf(v / step_) * step_ : v; }

  /// The slider band below the text (the bounds move while a Container scrolls).
  void syncSlider() {
    const Bounds& b = this->bounds;
    slider_.bounds = Bounds(b.x + 9, b.y + kTextHeight + 2, b.w - 18, 18);
  }
};

/// Caption on the left, Switch on the right; tapping the row toggles it.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class SwitchRow : public SettingRow<RGB_T> {
 public:
  static constexpr int32_t kHeight = 30;

  SwitchRow() = default;
  explicit SwitchRow(const char* caption) { this->setCaption(caption); }

  std::function<void(bool)> onChange;

  bool value() const { return switch_.value(); }
  void setValue(bool value) { switch_.setValue(value); }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    switch_.setTheme(theme);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    this->text(target, body, b.x, b.centerY() - static_cast<int32_t>(body.getHeight(1)) / 2, this->caption_.c_str(),
               this->dimmed(t.colors.onSurface));
    switch_.bounds = Bounds(b.right() - 44, b.centerY() - 12, 44, 24);
    switch_.enabled = this->enabled;
    switch_.draw(target);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type) || !this->enabled) return false;
    switch_.setValue(!switch_.value());
    if (onChange) onChange(switch_.value());
    return true;
  }

 private:
  Switch<RGB_T> switch_;
};

/// RadioButton and caption; tapping the row selects it.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class RadioRow : public SettingRow<RGB_T> {
 public:
  static constexpr int32_t kHeight = 26;

  RadioRow() = default;
  explicit RadioRow(const char* caption) { this->setCaption(caption); }

  /// Called when the row becomes selected by a tap.
  std::function<void()> onSelected;

  bool selected() const { return radio_.selected(); }
  void setSelected(bool selected) { radio_.setSelected(selected); }

  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    radio_.setTheme(theme);
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    radio_.bounds = Bounds(b.x, b.centerY() - 10, 20, 20);
    radio_.enabled = this->enabled;
    radio_.draw(target);
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    this->text(target, body, b.x + 26, b.centerY() - static_cast<int32_t>(body.getHeight(1)) / 2,
               this->caption_.c_str(), this->dimmed(t.colors.onSurface));
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type) || !this->enabled) return false;
    if (!radio_.selected()) {
      radio_.setSelected(true);
      if (onSelected) onSelected();
    }
    return true;
  }

 private:
  RadioButton<RGB_T> radio_{Bounds()};
};

/// Key on the left, value on the right (label font).
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class KeyValueRow : public SettingRow<RGB_T> {
 public:
  static constexpr int32_t kHeight = 14;

  KeyValueRow() = default;
  explicit KeyValueRow(const char* key, const char* value = "") : value_(value) { this->setCaption(key); }

  void setValue(const char* value) { value_ = value != nullptr ? value : ""; }
  const std::string& value() const { return value_; }
  void setValueColor(RGB_T color) {
    valueColor_ = color;
    hasValueColor_ = true;
  }
  void clearValueColor() { hasValueColor_ = false; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    tinygpu::IFont<RGB_T>& small = *t.typography.label;
    const int32_t y = b.centerY() - static_cast<int32_t>(small.getHeight(1)) / 2;
    this->text(target, small, b.x, y, this->caption_.c_str(), this->dimmed(t.colors.onSurfaceVariant));
    this->text(target, small, b.right() - static_cast<int32_t>(small.measureTextWidth(value_.c_str())), y,
               value_.c_str(), this->dimmed(hasValueColor_ ? valueColor_ : t.colors.onSurface));
  }

 private:
  std::string value_;
  RGB_T valueColor_{};
  bool hasValueColor_ = false;
};

}  // namespace tinymd
