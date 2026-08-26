#pragma once
#include "TinyGPU/Surface/ISurface.h"
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

/// Draws a line `thickness` times, offset by whole pixels, as a cheap
/// approximation of a stroked line - good enough at the small sizes these
/// vector icons are drawn at (checkbox/radio/icon-button glyphs).
template <typename RGB_T>
void drawLineThick(tinygpu::ISurface<RGB_T>& target, int32_t x0, int32_t y0,
                   int32_t x1, int32_t y1, RGB_T color, uint8_t thickness = 2) {
  if (thickness == 0) thickness = 1;
  const int32_t half = thickness / 2;
  for (int32_t o = -half; o < static_cast<int32_t>(thickness) - half; ++o) {
    target.drawLine(toPx(x0 + o), toPx(y0), toPx(x1 + o), toPx(y1), color);
    target.drawLine(toPx(x0), toPx(y0 + o), toPx(x1), toPx(y1 + o), color);
  }
}

/// Checkmark glyph (Checkbox, checked state), inset within `r`.
template <typename RGB_T>
void drawCheckmark(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
                   uint8_t thickness = 2) {
  const int32_t x0 = r.x + static_cast<int32_t>(r.w * 0.20f);
  const int32_t y0 = r.y + static_cast<int32_t>(r.h * 0.55f);
  const int32_t x1 = r.x + static_cast<int32_t>(r.w * 0.42f);
  const int32_t y1 = r.y + static_cast<int32_t>(r.h * 0.78f);
  const int32_t x2 = r.x + static_cast<int32_t>(r.w * 0.82f);
  const int32_t y2 = r.y + static_cast<int32_t>(r.h * 0.24f);
  drawLineThick(target, x0, y0, x1, y1, color, thickness);
  drawLineThick(target, x1, y1, x2, y2, color, thickness);
}

/// Filled dot (RadioButton, selected state), centered within `r`.
template <typename RGB_T>
void drawDot(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color) {
  const int32_t radius = r.w < r.h ? r.w / 4 : r.h / 4;
  target.fillCircle(toPx(r.centerX()), toPx(r.centerY()), toPx(radius), color);
}

/// "X" close glyph, inset within `r`.
template <typename RGB_T>
void drawClose(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
              uint8_t thickness = 2) {
  const Bounds in = r.inset(static_cast<int32_t>(r.w * 0.25f));
  drawLineThick(target, in.x, in.y, in.right(), in.bottom(), color, thickness);
  drawLineThick(target, in.x, in.bottom(), in.right(), in.y, color, thickness);
}

/// "+" plus glyph, inset within `r`.
template <typename RGB_T>
void drawPlus(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
             uint8_t thickness = 2) {
  const Bounds in = r.inset(static_cast<int32_t>(r.w * 0.22f));
  drawLineThick(target, in.centerX(), in.y, in.centerX(), in.bottom(), color, thickness);
  drawLineThick(target, in.x, in.centerY(), in.right(), in.centerY(), color, thickness);
}

/// "-" minus glyph, inset within `r`.
template <typename RGB_T>
void drawMinus(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
              uint8_t thickness = 2) {
  const Bounds in = r.inset(static_cast<int32_t>(r.w * 0.22f));
  drawLineThick(target, in.x, in.centerY(), in.right(), in.centerY(), color, thickness);
}

/// Downward chevron ("v"), inset within `r` - used for expandable affordances.
template <typename RGB_T>
void drawChevronDown(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
                     uint8_t thickness = 2) {
  const int32_t x0 = r.x + static_cast<int32_t>(r.w * 0.22f);
  const int32_t y0 = r.y + static_cast<int32_t>(r.h * 0.35f);
  const int32_t xm = r.centerX();
  const int32_t ym = r.y + static_cast<int32_t>(r.h * 0.65f);
  const int32_t x1 = r.x + static_cast<int32_t>(r.w * 0.78f);
  drawLineThick(target, x0, y0, xm, ym, color, thickness);
  drawLineThick(target, xm, ym, x1, y0, color, thickness);
}

/// Rightward chevron (">"), inset within `r` - navigation affordance.
template <typename RGB_T>
void drawChevronRight(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
                      uint8_t thickness = 2) {
  const int32_t x0 = r.x + static_cast<int32_t>(r.w * 0.35f);
  const int32_t y0 = r.y + static_cast<int32_t>(r.h * 0.22f);
  const int32_t xm = r.centerX();
  const int32_t ym = r.centerY();
  const int32_t y1 = r.y + static_cast<int32_t>(r.h * 0.78f);
  drawLineThick(target, x0, y0, xm, ym, color, thickness);
  drawLineThick(target, xm, ym, x0, y1, color, thickness);
}

/// Hamburger menu glyph (three horizontal bars), inset within `r` - the
/// conventional affordance for opening a Drawer.
template <typename RGB_T>
void drawMenu(tinygpu::ISurface<RGB_T>& target, const Bounds& r, RGB_T color,
             uint8_t thickness = 2) {
  const Bounds in = r.inset(static_cast<int32_t>(r.w * 0.15f));
  drawLineThick(target, in.x, in.y, in.right(), in.y, color, thickness);
  drawLineThick(target, in.x, in.centerY(), in.right(), in.centerY(), color, thickness);
  drawLineThick(target, in.x, in.bottom(), in.right(), in.bottom(), color, thickness);
}

}  // namespace tinymd
