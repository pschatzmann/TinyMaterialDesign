#pragma once
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyGPU/Font/LinePrinter.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/// Elevated rounded-rect container with an optional title and
/// word-wrapped body text.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Card : public Widget<RGB_T> {
 public:
  Card() = default;
  Card(Bounds bounds, const char* title = nullptr, const char* body = nullptr) {
    this->bounds = bounds;
    if (title != nullptr) setTitle(title);
    if (body != nullptr) setBody(body);
  }

  /// Cheap elevation approximation: an offset, darker rounded rect drawn
  /// behind the card (see MaterialTheme's blend()). Set false for a flat card.
  bool elevated = true;

  void setTitle(const char* title) {
    title_ = title;
    hasTitle_ = true;
  }
  void setBody(const char* body) {
    body_ = body;
    hasBody_ = true;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const size_t radius = toPx(this->theme().shape.medium);

    if (elevated) {
      const RGB_T shadow = blend(this->theme().colors.surface, this->theme().colors.onBackground, 0.30f);
      target.fillRoundRect(toPx(this->bounds.x + 1), toPx(this->bounds.y + 2),
                           toPx(this->bounds.w), toPx(this->bounds.h), radius, shadow);
    }

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.surface);
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, this->theme().colors.outline);

    const int32_t pad = this->theme().spacing;
    int32_t cursorY = this->bounds.y + pad;

    if (hasTitle_) {
      tinygpu::IFont<RGB_T>& titleFont = *this->theme().typography.title;
      titleFont.drawText(target, static_cast<int16_t>(this->bounds.x + pad),
                         static_cast<int16_t>(cursorY), title_.c_str(), this->theme().colors.onSurface,
                         this->theme().colors.surface, false);
      cursorY += static_cast<int32_t>(titleFont.getHeight(1)) + pad / 2;
    }

    if (hasBody_) {
      // LinePrinter's borders are measured from the *target surface's*
      // edges (it's designed for "print with margins", not "wrap inside an
      // arbitrary rect") - so the right border has to be back-computed from
      // the card's right edge rather than set directly.
      const int32_t rightEdge = this->bounds.right() - pad;
      const size_t rightBorder =
          rightEdge < static_cast<int32_t>(target.width())
              ? target.width() - static_cast<size_t>(rightEdge)
              : 0;

      tinygpu::LinePrinter<RGB_T> printer;
      printer.setFont(*this->theme().typography.body);
      printer.setTarget(target);
      printer.setColor(this->theme().colors.onSurfaceVariant);
      printer.setTopBorder(toPx(cursorY));
      printer.setLeftBorder(toPx(this->bounds.x + pad));
      printer.setRightBorder(rightBorder);
      printer.print(body_.c_str());
    }
  }

 private:
  std::string title_;
  std::string body_;
  bool hasTitle_ = false;
  bool hasBody_ = false;
};

using CardRGB565 = Card<tinygpu::RGB565>;
using CardRGB666 = Card<tinygpu::RGB666>;
using CardRGB888 = Card<tinygpu::RGB888>;

}  // namespace tinymd
