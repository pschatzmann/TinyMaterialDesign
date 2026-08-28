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

/// Darkens every pixel already drawn across `target`'s full extent, in
/// place - the modal scrim look shared by Dialog and Drawer. Reads each
/// pixel back and blends it 35% toward black, rather than painting a
/// flat precomputed color over everything: this format has no alpha
/// channel, so a flat fill would just erase whatever's actually behind
/// the modal (the real screen content) instead of dimming it. Relies on
/// `target` already holding that real content when called - true for
/// Screen::draw()'s single-buffer pass (widgets are drawn before the
/// modal), and for Screen::drawDirect()'s modal band loop once it draws
/// the regular content into each band first (see drawModalDirect()).
template <typename RGB_T>
void drawScrim(tinygpu::ISurface<RGB_T>& target) {
  const size_t w = target.width();
  const size_t h = target.height();
  for (size_t y = 0; y < h; ++y) {
    for (size_t x = 0; x < w; ++x) {
      target.setPixel(x, y, blend(target.getPixel(x, y), RGB_T(0, 0, 0), 0.35f));
    }
  }
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

  /// Sets the theme this widget draws itself with - see theme(). Screen
  /// pushes its own theme into a widget at registration time
  /// (addWidget()/addFixedWidget()/presentDialog()) and again whenever
  /// Screen::setTheme() is called; container widgets (Dialog, Drawer)
  /// cascade this down to whatever children they already hold.
  virtual void setTheme(const MaterialTheme<RGB_T>& theme) { theme_ = &theme; }

  /// Renders the widget into `target` using colors/fonts/tokens from its
  /// own theme (see theme()/setTheme()).
  virtual void draw(tinygpu::ISurface<RGB_T>& target) = 0;

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

  /// Number of child widgets this one draws as part of itself, if any (0
  /// by default) - a composite widget like Drawer overrides this + child()
  /// to expose its items (Dialog its actions, etc.), so
  /// Screen::drawDirect() can draw a presented modal's children
  /// individually, each into its own small right-sized buffer, instead of
  /// needing one buffer big enough for the whole modal - see
  /// drawBackground().
  virtual int childCount() const { return 0; }

  /// The `index`-th child (see childCount()); never called with an
  /// out-of-range index. Not owned by this widget any more than by
  /// whatever added it (addItem()/addAction()/...).
  virtual Widget<RGB_T>* child(int index) {
    (void)index;
    return nullptr;
  }

  /// Draws everything this widget owns *except* its children (see
  /// childCount()) - e.g. Drawer's scrim + panel fill, without its list
  /// items. Default: draws everything via draw() itself, which is exactly
  /// right for a widget that doesn't override childCount() (its "children"
  /// are however many it reports - zero - so nothing is skipped).
  /// Screen::drawDirect() calls this instead of draw() for a presented
  /// modal, then draws each child (if any) separately.
  virtual void drawBackground(tinygpu::ISurface<RGB_T>& target) {
    draw(target);
  }

  /// Lets a container (AppBar, chiefly) push its own background/foreground
  /// color pair down onto a child it draws itself (leading/trailing), so
  /// e.g. an IconButton in an app bar with a color override reads clearly
  /// against it by default instead of staying on its own theme-wide
  /// default (which is tuned to sit on theme.colors.surface, not
  /// whatever color the app bar happens to be). Default: no-op, correct
  /// for any widget with no notion of "ambient" vs. its own explicit
  /// styling. IconButton overrides this as a fallback *only* used when it
  /// has no explicit setColorOverride() of its own, so e.g. a record
  /// button's active-red override still wins over the app bar's ambient
  /// tint.
  virtual void setThemeTint(RGB_T background, RGB_T foreground) {
    (void)background;
    (void)foreground;
  }

 protected:
  /// The theme this widget draws itself with - set via setTheme(), pushed
  /// in by Screen at registration time. Never dereferenced before that -
  /// every widget is registered with a Screen (which always has a theme,
  /// default-constructed if nothing else) before its first draw().
  const MaterialTheme<RGB_T>* theme_ = nullptr;

  /// Convenience accessor for draw() overrides - see theme_.
  const MaterialTheme<RGB_T>& theme() const { return *theme_; }
};

}  // namespace tinymd
