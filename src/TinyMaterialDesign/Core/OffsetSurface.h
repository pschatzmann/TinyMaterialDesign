#pragma once
#include <stdint.h>

#include "TinyGPU/Surface/ISurface.h"
#include "TinyGPU/Surface/SurfaceBase.h"

namespace tinymd {

/**
 * @brief Draws into `target` moved up by `dy` pixels, only inside the rows
 * [clipTop, clipBottom) of `target`.
 *
 * TinyGPU's drawing primitives take unsigned coordinates, and toPx() clamps
 * negative values to 0: a widget whose top is above the edge of the surface
 * (e.g. scrolled up in a Container) would be drawn at y = 0 with its full
 * height instead of partly hidden. Container draws such a widget through an
 * OffsetSurface: the widget is moved down by `dy` (so all its coordinates
 * are >= 0) and every pixel is written `dy` rows higher, where it belongs.
 * Pixel by pixel (via SurfaceBase), so only used for those widgets.
 */
template <typename RGB_T>
class OffsetSurface : public tinygpu::SurfaceBase<RGB_T> {
 public:
  OffsetSurface(tinygpu::ISurface<RGB_T>& target, int32_t dy, int32_t clipTop, int32_t clipBottom)
      : tinygpu::SurfaceBase<RGB_T>(target.width(), target.height() + static_cast<size_t>(dy), target.font()),
        target_(target),
        dy_(dy),
        clipTop_(clipTop),
        clipBottom_(clipBottom) {}

  void setPixel(size_t x, size_t y, RGB_T color) override {
    const int32_t ty = static_cast<int32_t>(y) - dy_;
    if (ty < clipTop_ || ty >= clipBottom_ || ty < 0 || x >= target_.width()) return;
    target_.setPixel(x, static_cast<size_t>(ty), color);
  }

  RGB_T getPixel(size_t x, size_t y) const override {
    const int32_t ty = static_cast<int32_t>(y) - dy_;
    if (ty < 0 || static_cast<size_t>(ty) >= target_.height() || x >= target_.width()) return RGB_T();
    return target_.getPixel(x, static_cast<size_t>(ty));
  }

  bool resizeBuffer(size_t, size_t) override { return true; }
  const uint8_t* data() const override { return nullptr; }
  size_t size() const override { return 0; }

 private:
  tinygpu::ISurface<RGB_T>& target_;
  int32_t dy_;
  int32_t clipTop_;
  int32_t clipBottom_;
};

}  // namespace tinymd
