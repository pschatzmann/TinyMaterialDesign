#pragma once
#include <stdint.h>

#include "TinyGPU/Input/GestureDetector.h"
#include "TinyGPU/Surface/ISurface.h"
#include "TinyMaterialDesignConfig.h"
#include "TinyMaterialDesign/Core/Bounds.h"
#include "TinyMaterialDesign/Theme/MaterialTheme.h"

namespace tinymd {

/// True for GestureType::kTap and also kDoubleTap - widgets that react to a
/// plain tap should check this instead of comparing against kTap directly.
///
/// Why: GestureDetector reclassifies a tap as kDoubleTap whenever it lands
/// within doubleTapMaxGapMs (350ms by default) of *any* previous tap, not
/// just a second tap on the same widget - so quickly tapping one key then
/// another on Keyboard (completely normal while typing) can turn the second
/// one into a kDoubleTap. A widget that only matches kTap exactly then
/// silently drops that keypress, which is exactly the "the Done key doesn't
/// work reliably" symptom this fixes. None of these widgets give a tap and
/// a double-tap different meaning, so treating both as "the user tapped me"
/// is the correct behavior everywhere a plain tap is handled.
inline bool isTapGesture(tinygpu::GestureType type) {
  return type == tinygpu::GestureType::kTap || type == tinygpu::GestureType::kDoubleTap;
}

/**
 * @brief Base class every TinyMaterialDesign widget derives from.
 *
 * Widgets are plain templated objects the sketch owns (as globals or
 * members, same non-owning-reference style TinyGPU itself uses) and
 * registers with a Screen via Screen::addWidget(). Screen calls draw()/
 * update()/onGesture() on the sketch's behalf each loop() iteration.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Widget {
 public:
  virtual ~Widget() = default;

  Bounds bounds;
  bool visible = true;
  bool enabled = true;

  /// Renders the widget into `target` using colors/fonts/tokens from
  /// `theme`.
  virtual void draw(tinygpu::ISurface<RGB_T>& target,
                    const MaterialTheme<RGB_T>& theme) = 0;

  /// Handles one gesture event. Returns true if this widget consumed it.
  virtual bool onGesture(const tinygpu::GestureEvent& event) {
    (void)event;
    return false;
  }

  /// Called once per frame, before draw(), to advance any time-based
  /// animation (ripple fade, indeterminate progress sweep, ...). Returns
  /// true if that advance changed anything this widget will draw
  /// differently, so Screen can decide whether a redraw is actually needed -
  /// see Screen::isDirty(). Default: no-op, not dirty, so widgets that
  /// don't animate cost nothing extra.
  virtual bool update(uint32_t nowMs) {
    (void)nowMs;
    return false;
  }

  /// Whether a continuous drag starting inside this widget's bounds should
  /// be classified as GestureType::kDrag (see GestureDetector::isDraggable
  /// and Screen::isDraggableAt). Only Slider needs true.
  virtual bool isDraggable() const { return false; }
};

}  // namespace tinymd
