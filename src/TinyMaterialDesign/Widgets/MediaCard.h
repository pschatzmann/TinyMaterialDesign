#pragma once
#include <functional>
#include <string>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Tappable card with a thumbnail image and a caption - e.g. one tile
 * in a grid of stations/genres/countries. Pair with GridLayout
 * (Core/GridLayout.h) to lay out several of these.
 *
 * The image is any TinyGPU ISurface<RGB_T> (a Sprite, a decoded BMP, ...)
 * you already have - this widget only blits it via ISurface::drawSprite(),
 * it doesn't fetch or decode anything itself (TinyGPU has no HTTP/network
 * layer). Not owned: pass a pointer that outlives the card, or nullptr for
 * a placeholder fill. There's no scaling either - drawSprite() blits 1:1,
 * so pre-size the image to fit the card's image area (bounds minus the
 * caption strip and padding) before handing it over.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class MediaCard : public Widget<RGB_T> {
 public:
  MediaCard() = default;
  MediaCard(Bounds bounds, const char* caption = nullptr) {
    this->bounds = bounds;
    if (caption != nullptr) setCaption(caption);
  }

  std::function<void()> onClick;

  /// Not owned - must outlive the card, or be reset to nullptr/another
  /// image before it doesn't. nullptr draws a plain placeholder fill.
  void setImage(const tinygpu::ISurface<RGB_T>* image) { image_ = image; }
  void setCaption(const char* caption) { caption_ = caption; }

  /// "Now playing"/highlighted state: filled tint + primary-colored outline.
  bool selected() const { return selected_; }
  void setSelected(bool selected) { selected_ = selected; }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) override {
    const size_t radius = toPx(theme.shape.medium);
    RGB_T background = selected_ ? theme.colors.secondaryContainer : theme.colors.surface;
    RGB_T outline = selected_ ? theme.colors.primary : theme.colors.outline;
    RGB_T captionColor = selected_ ? theme.colors.onSecondaryContainer : theme.colors.onSurface;
    if (!this->enabled) {
      background = blend(background, theme.colors.surface, 0.5f);
      outline = blend(outline, theme.colors.surface, 0.5f);
      captionColor = blend(captionColor, theme.colors.surface, 0.5f);
    }

    target.fillRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, background);
    target.drawRoundRect(toPx(this->bounds.x), toPx(this->bounds.y), toPx(this->bounds.w),
                         toPx(this->bounds.h), radius, outline);

    const int32_t pad = 4;
    const int32_t captionHeight = 18;
    const Bounds imageArea(this->bounds.x + pad, this->bounds.y + pad, this->bounds.w - 2 * pad,
                           this->bounds.h - captionHeight - 2 * pad);

    if (image_ != nullptr) {
      target.drawSprite(toPx(imageArea.x), toPx(imageArea.y), *image_);
    } else if (imageArea.w > 0 && imageArea.h > 0) {
      target.fillRect(toPx(imageArea.x), toPx(imageArea.y), toPx(imageArea.w), toPx(imageArea.h),
                      theme.colors.surfaceVariant);
    }

    tinygpu::IFont<RGB_T>& font = *theme.typography.label;
    const size_t textWidth = font.measureTextWidth(caption_.c_str());
    const int32_t textX = this->bounds.centerX() - static_cast<int32_t>(textWidth) / 2;
    const int32_t textY = this->bounds.bottom() - captionHeight - pad + 2;
    font.drawText(target, static_cast<int16_t>(textX), static_cast<int16_t>(textY),
                 caption_.c_str(), captionColor, background, false);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (onClick) onClick();
    return true;
  }

 private:
  const tinygpu::ISurface<RGB_T>* image_ = nullptr;
  std::string caption_;
  bool selected_ = false;
};

using MediaCardRGB565 = MediaCard<tinygpu::RGB565>;
using MediaCardRGB666 = MediaCard<tinygpu::RGB666>;
using MediaCardRGB888 = MediaCard<tinygpu::RGB888>;

}  // namespace tinymd
