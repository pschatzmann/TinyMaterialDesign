#pragma once
#include <stdint.h>
#include <algorithm>
#include <cmath>

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

/// Builds a color from HSL (h in degrees, s/l in [0, 1]) - lets the seeded
/// schemes below express roles as "same hue, tuned tone/chroma" instead of
/// hand-picked hex constants per role.
template <typename RGB_T>
inline RGB_T colorFromHsl(float h, float s, float l) {
  h = std::fmod(h, 360.0f);
  if (h < 0.0f) h += 360.0f;
  s = std::min(1.0f, std::max(0.0f, s));
  l = std::min(1.0f, std::max(0.0f, l));

  const float chroma = (1.0f - std::fabs(2.0f * l - 1.0f)) * s;
  const float hp = h / 60.0f;
  const float x = chroma * (1.0f - std::fabs(std::fmod(hp, 2.0f) - 1.0f));
  float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
  if (hp < 1.0f) { r1 = chroma; g1 = x; }
  else if (hp < 2.0f) { r1 = x; g1 = chroma; }
  else if (hp < 3.0f) { g1 = chroma; b1 = x; }
  else if (hp < 4.0f) { g1 = x; b1 = chroma; }
  else if (hp < 5.0f) { r1 = x; b1 = chroma; }
  else { r1 = chroma; b1 = x; }
  const float m = l - chroma / 2.0f;
  auto to8 = [m](float v) { return static_cast<uint8_t>(std::round((v + m) * 255.0f)); };
  return RGB_T(to8(r1), to8(g1), to8(b1));
}

/// Derives a full Material 3 role set from one seed hue, following the same
/// tone relationships as the baseline purple scheme above (bold primary,
/// pale/dark container pair, near-neutral surface) rather than pulling exact
/// values from a specific Google-published palette. Handy directly for a
/// fully custom accent color, and backs the named presets below.
template <typename RGB_T>
ColorScheme<RGB_T> colorSchemeFromHue(float hueDegrees, bool dark) {
  ColorScheme<RGB_T> c;
  if (!dark) {
    c.primary = colorFromHsl<RGB_T>(hueDegrees, 0.55f, 0.40f);
    c.onPrimary = colorFromHex<RGB_T>(0xFFFFFF);
    c.primaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.70f, 0.90f);
    c.onPrimaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.55f, 0.12f);

    c.secondary = colorFromHsl<RGB_T>(hueDegrees, 0.15f, 0.40f);
    c.onSecondary = colorFromHex<RGB_T>(0xFFFFFF);
    c.secondaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.20f, 0.90f);
    c.onSecondaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.20f, 0.12f);

    c.error = colorFromHex<RGB_T>(0xB3261E);
    c.onError = colorFromHex<RGB_T>(0xFFFFFF);

    c.background = colorFromHsl<RGB_T>(hueDegrees, 0.05f, 0.99f);
    c.onBackground = colorFromHsl<RGB_T>(hueDegrees, 0.05f, 0.11f);
    c.surface = c.background;
    c.onSurface = c.onBackground;
    c.surfaceVariant = colorFromHsl<RGB_T>(hueDegrees, 0.10f, 0.90f);
    c.onSurfaceVariant = colorFromHsl<RGB_T>(hueDegrees, 0.06f, 0.30f);

    c.outline = colorFromHsl<RGB_T>(hueDegrees, 0.05f, 0.48f);
  } else {
    c.primary = colorFromHsl<RGB_T>(hueDegrees, 0.60f, 0.80f);
    c.onPrimary = colorFromHsl<RGB_T>(hueDegrees, 0.45f, 0.20f);
    c.primaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.55f, 0.30f);
    c.onPrimaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.70f, 0.90f);

    c.secondary = colorFromHsl<RGB_T>(hueDegrees, 0.20f, 0.80f);
    c.onSecondary = colorFromHsl<RGB_T>(hueDegrees, 0.15f, 0.20f);
    c.secondaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.20f, 0.30f);
    c.onSecondaryContainer = colorFromHsl<RGB_T>(hueDegrees, 0.20f, 0.90f);

    c.error = colorFromHex<RGB_T>(0xF2B8B5);
    c.onError = colorFromHex<RGB_T>(0x601410);

    c.background = colorFromHsl<RGB_T>(hueDegrees, 0.08f, 0.10f);
    c.onBackground = colorFromHsl<RGB_T>(hueDegrees, 0.08f, 0.90f);
    c.surface = c.background;
    c.onSurface = c.onBackground;
    c.surfaceVariant = colorFromHsl<RGB_T>(hueDegrees, 0.10f, 0.30f);
    c.onSurfaceVariant = colorFromHsl<RGB_T>(hueDegrees, 0.06f, 0.80f);

    c.outline = colorFromHsl<RGB_T>(hueDegrees, 0.05f, 0.60f);
  }
  return c;
}

/// Named seed-hue presets, each a light/dark pair - pass to MaterialTheme's
/// blueTheme()/greenTheme()/redTheme()/orangeTheme() (and their *DarkTheme()
/// counterparts) in MaterialTheme.h rather than calling these directly.
template <typename RGB_T>
ColorScheme<RGB_T> blueColorScheme() { return colorSchemeFromHue<RGB_T>(210.0f, false); }
template <typename RGB_T>
ColorScheme<RGB_T> blueDarkColorScheme() { return colorSchemeFromHue<RGB_T>(210.0f, true); }

template <typename RGB_T>
ColorScheme<RGB_T> greenColorScheme() { return colorSchemeFromHue<RGB_T>(140.0f, false); }
template <typename RGB_T>
ColorScheme<RGB_T> greenDarkColorScheme() { return colorSchemeFromHue<RGB_T>(140.0f, true); }

template <typename RGB_T>
ColorScheme<RGB_T> redColorScheme() { return colorSchemeFromHue<RGB_T>(10.0f, false); }
template <typename RGB_T>
ColorScheme<RGB_T> redDarkColorScheme() { return colorSchemeFromHue<RGB_T>(10.0f, true); }

template <typename RGB_T>
ColorScheme<RGB_T> orangeColorScheme() { return colorSchemeFromHue<RGB_T>(30.0f, false); }
template <typename RGB_T>
ColorScheme<RGB_T> orangeDarkColorScheme() { return colorSchemeFromHue<RGB_T>(30.0f, true); }

}  // namespace tinymd
