#pragma once
#include <algorithm>

#include "TinyMaterialDesignConfig.h"
#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyGPU/Fonts.h"
#include "TinyMaterialDesign/Theme/MaterialColors.h"

namespace tinymd {

/// Per-channel linear blend between two colors (t=0 -> a, t=1 -> b), used
/// for pressed/disabled state shading, cheap elevation "shadows", and the
/// ripple fade. RGB_T has no native alpha channel, so this is the cheapest
/// approximation that still looks right at typical embedded-UI sizes.
template <typename RGB_T>
RGB_T blend(RGB_T a, RGB_T b, float t) {
  t = std::min(1.0f, std::max(0.0f, t));
  auto lerp = [t](uint8_t x, uint8_t y) {
    return static_cast<uint8_t>(x + (static_cast<float>(y) - x) * t);
  };
  return RGB_T(lerp(a.getRed(), b.getRed()), lerp(a.getGreen(), b.getGreen()),
              lerp(a.getBlue(), b.getBlue()));
}

/// Material 3 shape-scale corner radii, in pixels.
struct ShapeTokens {
  int32_t small = 4;
  int32_t medium = 8;
  int32_t large = 16;
  /// "Fully rounded" (pills, circular thumbs) - callers pass this straight
  /// into fillRoundRect()/drawRoundRect(), which clamp it to min(w,h)/2, so
  /// any generously-large constant works regardless of the widget's size.
  int32_t full = 1000;
};

/// Typography roles, each backed by one of TinyGPU's bitmap fonts.
template <typename RGB_T>
struct Typography {
  tinygpu::IFont<RGB_T>* label;     // Font5x7 - buttons, chips, captions
  tinygpu::IFont<RGB_T>* body;      // Font8x8 - card/dialog body text
  tinygpu::IFont<RGB_T>* title;     // Font12x12 - card/dialog/app bar titles
  tinygpu::IFont<RGB_T>* headline;  // Font16x24 - large standalone headings
};

/// @brief Bundles a color scheme, shape tokens, typography, and spacing into
/// one theme, passed to widgets so they all draw with consistent styling.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
struct MaterialTheme {
  ColorScheme<RGB_T> colors;
  ShapeTokens shape;
  Typography<RGB_T> typography;
  int32_t spacing = 8;

  /// Grows/fades a circle from the touch point on Button taps. Set false on
  /// very constrained targets to skip the per-frame update()/redraw cost.
  bool enableRipple = true;
};

/// Function-local static fonts, constructed once on first use - the same
/// "kept as static instances" idiom TinyGPU's own basic-example.ino uses,
/// just centralized here instead of left to each sketch.
template <typename RGB_T>
Typography<RGB_T> defaultTypography() {
  static tinygpu::Font5x7<RGB_T> label;
  static tinygpu::Font8x8<RGB_T> body;
  static tinygpu::Font12x12<RGB_T> title;
  static tinygpu::Font16x24<RGB_T> headline;
  return Typography<RGB_T>{&label, &body, &title, &headline};
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> defaultTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = lightColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> defaultDarkTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = darkColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

/// Additional named seed-hue themes (see MaterialColors.h's
/// colorSchemeFromHue()) - same shape as defaultTheme()/defaultDarkTheme(),
/// just with a different accent hue baked into the color scheme.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> blueTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = blueColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> blueDarkTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = blueDarkColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> greenTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = greenColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> greenDarkTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = greenDarkColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> redTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = redColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> redDarkTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = redDarkColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> orangeTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = orangeColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
MaterialTheme<RGB_T> orangeDarkTheme() {
  MaterialTheme<RGB_T> theme;
  theme.colors = orangeDarkColorScheme<RGB_T>();
  theme.typography = defaultTypography<RGB_T>();
  return theme;
}

using MaterialThemeRGB565 = MaterialTheme<tinygpu::RGB565>;
using MaterialThemeRGB666 = MaterialTheme<tinygpu::RGB666>;
using MaterialThemeRGB888 = MaterialTheme<tinygpu::RGB888>;

}  // namespace tinymd
