#pragma once
#include <string>

#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Widgets/Label.h"

namespace tinymd {

/**
 * @brief A small tile with one value: a caption at the top, the value in an
 * accent color and an optional unit / sub text - for dashboards and KPIs.
 *
 *   StatTile<RGB565> rate(Bounds(8, 40, 72, 46), "RATE", "s/day");
 *   rate.setValue("+4.2");
 *   rate.setAccent(colorFromHex<RGB565>(0x10B981));
 *
 * The value is drawn in the title font and falls back to the body and label
 * fonts if it does not fit. Outlined (default, on the page) or tonal (filled
 * with surfaceVariant, e.g. inside a Card).
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class StatTile : public Widget<RGB_T> {
 public:
  StatTile() = default;
  explicit StatTile(const char* caption, const char* sub = "") : caption_(caption), sub_(sub) {}
  StatTile(Bounds bounds, const char* caption, const char* sub = "") : caption_(caption), sub_(sub) {
    this->bounds = bounds;
  }

  void setCaption(const char* caption) { caption_ = caption; }
  void setValue(const char* value) { value_ = value; }
  const std::string& value() const { return value_; }
  void setSub(const char* sub) { sub_ = sub; }
  void setAccent(RGB_T color) {
    accent_ = color;
    hasAccent_ = true;
  }
  /// Filled with surfaceVariant instead of outlined.
  bool tonal = false;
  /// Font of the value (default: title, which falls back to smaller fonts).
  TypographyRole valueRole = TypographyRole::kTitle;

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    const RGB_T bg = tonal ? t.colors.surfaceVariant : t.colors.surface;
    const size_t radius = toPx(t.shape.medium);
    target.fillRoundRect(toPx(b.x), toPx(b.y), toPx(b.w), toPx(b.h), radius, bg);
    if (!tonal) target.drawRoundRect(toPx(b.x), toPx(b.y), toPx(b.w), toPx(b.h), radius, t.colors.outline);

    tinygpu::IFont<RGB_T>& small = *t.typography.label;
    const int32_t x = b.x + 5;
    small.drawText(target, static_cast<int16_t>(x), static_cast<int16_t>(b.y + 4), caption_.c_str(),
                   t.colors.onSurfaceVariant, bg, false);

    // the largest font that fits
    tinygpu::IFont<RGB_T>* fonts[3] = {fontForTypographyRole(valueRole, t), t.typography.body, t.typography.label};
    tinygpu::IFont<RGB_T>* font = fonts[2];
    for (tinygpu::IFont<RGB_T>* f : fonts) {
      if (static_cast<int32_t>(f->measureTextWidth(value_.c_str())) <= b.w - 8) {
        font = f;
        break;
      }
    }
    const int32_t smallH = static_cast<int32_t>(small.getHeight(1));
    const int32_t valueY = sub_.empty() ? b.bottom() - 5 - static_cast<int32_t>(font->getHeight(1))
                                        : b.y + 4 + smallH + 3;
    font->drawText(target, static_cast<int16_t>(x), static_cast<int16_t>(valueY), value_.c_str(),
                   hasAccent_ ? accent_ : t.colors.primary, bg, false);
    if (!sub_.empty()) {
      small.drawText(target, static_cast<int16_t>(x), static_cast<int16_t>(b.bottom() - 4 - smallH), sub_.c_str(),
                     t.colors.onSurfaceVariant, bg, false);
    }
  }

 private:
  std::string caption_;
  std::string value_ = "--";
  std::string sub_;
  RGB_T accent_{};
  bool hasAccent_ = false;
};

}  // namespace tinymd
