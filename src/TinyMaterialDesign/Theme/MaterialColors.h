#pragma once
#include <stdint.h>

namespace tinymd {

/// Packs a 0xRRGGBB hex constant into an RGB_T (RGB565/RGB666/RGB888 all
/// share the same (r, g, b) 8-bit-per-channel constructor).
template <typename RGB_T>
inline RGB_T colorFromHex(uint32_t hex) {
  return RGB_T(static_cast<uint8_t>((hex >> 16) & 0xFF),
              static_cast<uint8_t>((hex >> 8) & 0xFF),
              static_cast<uint8_t>(hex & 0xFF));
}

/**
 * @brief Material Design 3 color roles.
 *
 * Every widget draws using roles from this struct rather than hard-coded
 * colors, so swapping a Screen's theme re-colors every widget on it.
 */
template <typename RGB_T>
struct ColorScheme {
  RGB_T primary;
  RGB_T onPrimary;
  RGB_T primaryContainer;
  RGB_T onPrimaryContainer;

  RGB_T secondary;
  RGB_T onSecondary;
  RGB_T secondaryContainer;
  RGB_T onSecondaryContainer;

  RGB_T error;
  RGB_T onError;

  RGB_T background;
  RGB_T onBackground;

  RGB_T surface;
  RGB_T onSurface;
  RGB_T surfaceVariant;
  RGB_T onSurfaceVariant;

  RGB_T outline;
};

/// Material 3 "Baseline" light color scheme (the reference purple palette
/// Google publishes for M3).
template <typename RGB_T>
ColorScheme<RGB_T> lightColorScheme() {
  ColorScheme<RGB_T> c;
  c.primary = colorFromHex<RGB_T>(0x6750A4);
  c.onPrimary = colorFromHex<RGB_T>(0xFFFFFF);
  c.primaryContainer = colorFromHex<RGB_T>(0xEADDFF);
  c.onPrimaryContainer = colorFromHex<RGB_T>(0x21005D);

  c.secondary = colorFromHex<RGB_T>(0x625B71);
  c.onSecondary = colorFromHex<RGB_T>(0xFFFFFF);
  c.secondaryContainer = colorFromHex<RGB_T>(0xE8DEF8);
  c.onSecondaryContainer = colorFromHex<RGB_T>(0x1D192B);

  c.error = colorFromHex<RGB_T>(0xB3261E);
  c.onError = colorFromHex<RGB_T>(0xFFFFFF);

  c.background = colorFromHex<RGB_T>(0xFFFBFE);
  c.onBackground = colorFromHex<RGB_T>(0x1C1B1F);

  c.surface = colorFromHex<RGB_T>(0xFFFBFE);
  c.onSurface = colorFromHex<RGB_T>(0x1C1B1F);
  c.surfaceVariant = colorFromHex<RGB_T>(0xE7E0EC);
  c.onSurfaceVariant = colorFromHex<RGB_T>(0x49454F);

  c.outline = colorFromHex<RGB_T>(0x79747E);
  return c;
}

/// Material 3 "Baseline" dark color scheme.
template <typename RGB_T>
ColorScheme<RGB_T> darkColorScheme() {
  ColorScheme<RGB_T> c;
  c.primary = colorFromHex<RGB_T>(0xD0BCFF);
  c.onPrimary = colorFromHex<RGB_T>(0x381E72);
  c.primaryContainer = colorFromHex<RGB_T>(0x4F378B);
  c.onPrimaryContainer = colorFromHex<RGB_T>(0xEADDFF);

  c.secondary = colorFromHex<RGB_T>(0xCCC2DC);
  c.onSecondary = colorFromHex<RGB_T>(0x332D41);
  c.secondaryContainer = colorFromHex<RGB_T>(0x4A4458);
  c.onSecondaryContainer = colorFromHex<RGB_T>(0xE8DEF8);

  c.error = colorFromHex<RGB_T>(0xF2B8B5);
  c.onError = colorFromHex<RGB_T>(0x601410);

  c.background = colorFromHex<RGB_T>(0x1C1B1F);
  c.onBackground = colorFromHex<RGB_T>(0xE6E1E5);

  c.surface = colorFromHex<RGB_T>(0x1C1B1F);
  c.onSurface = colorFromHex<RGB_T>(0xE6E1E5);
  c.surfaceVariant = colorFromHex<RGB_T>(0x49454F);
  c.onSurfaceVariant = colorFromHex<RGB_T>(0xCAC4D0);

  c.outline = colorFromHex<RGB_T>(0x938F99);
  return c;
}

}  // namespace tinymd
