#pragma once
#include "TinyGPU/Boards/LCDBoards.h"
#include "TinyGPU/Drivers/DeviceOutput.h"
#include "TinyGPU/Input/GestureDetector.h"
#include "TinyGPU/Surface/Surface.h"
#include "TinyMaterialDesignConfig.h"
#include "TinyMaterialDesign/Core/Screen.h"
#include "TinyMaterialDesign/Theme/MaterialTheme.h"

namespace tinymd {

/**
 * @brief Bundles the surface + display + gesture routing + Screen
 * boilerplate that every single-widget example under examples/controls and
 * examples/layouts otherwise hand-rolls identically, down to a constructor
 * plus two calls: begin() and update().
 *
 * Without `App`, every one of those sketches repeats the same ~15 lines:
 *
 *   Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
 *   DeviceOutput<RGB565> display(board.display());
 *   GestureDetector gestures;
 *   Screen<RGB565> screen(theme);
 *   // ...then in setup(): display.begin(); surface.begin();
 *   // gestures.onGesture = ...; gestures.isDraggable = ...;
 *   // ...then in loop(): gestures.update(*board.touch()); screen.update(...);
 *   // if (screen.isDirty()) { screen.draw(surface); display.writeData(surface); }
 *
 * `App` is exactly that, reduced to:
 * 
 *   LCDBoardGuitionESP32_LVGL_2_4Display board;
 *   App<RGB565> app(board);
 *   Button<RGB565> myButton(Bounds(...), "Tap me");
 *
 *   void setup() {
 *     app.begin();
 *     myButton.onClick = []() { ... };
 *     app.screen().addWidget(myButton);
 *   }
 *
 *   void loop() {
 *     app.update();
 *   }
 *
 * The board itself is left to the sketch, not `App` - board *types* differ
 * per target (a desktop board takes width/height in its constructor, most
 * real hardware boards take none, and a board with nonstandard wiring
 * takes pin numbers) enough that there's no one constructor `App` could
 * call on your behalf - `TinyMaterialDesignConfig.h`'s `DefaultBoard` is a
 * ready-made object of whichever concrete type fits the current platform,
 * for exactly this reason (see its own doc comment); pass a different
 * `LCDBoard` instead for anything else. Whatever you pass must already be
 * fully constructed and must outlive this `App` - the same
 * non-owning-reference convention every other TinyMaterialDesign class
 * uses. Width/height are read from `board.width()`/`board.height()`, so
 * they don't need to be passed again.
 *
 * Pinned to `LCDBoard`'s own RGB565-only `display()` (see
 * `TinyGPU/Boards/LCDBoardsCommon.h`'s class comment) - `RGB_T` is
 * templated for consistency with every other class in this library, but in
 * practice only `RGB565` compiles today, the same constraint every
 * hand-rolled example already has.
 *
 * Only one `App` may be alive at a time (per `RGB_T` instantiation): the
 * gesture callbacks it wires up in begin() are plain C function pointers
 * (`GestureDetector::onGesture`/`isDraggable`, not `std::function`), so
 * they can't capture `this` - `App` routes them through a static
 * self-pointer instead. Every example this replaces already only ever
 * created one `Screen`/`GestureDetector` pair as globals, so this isn't a
 * new restriction in practice.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class App {
 public:
  /// `board` must already be fully constructed (with whatever constructor
  /// arguments its concrete type needs) and must outlive this `App` - see
  /// the class comment.
  explicit App(tinygpu::LCDBoard& board, const MaterialTheme<RGB_T>& theme = defaultTheme<RGB_T>())
      : board_(board),
        surface_(board.width(), board.height(), tinygpu::FontRGB565),
        display_(board.display()),
        screen_(theme) {
    instance_ = this;
  }

  /// Starts the board, display, and surface, and wires gesture routing to
  /// the Screen - call once from setup(), before or after registering
  /// widgets via screen() (order between the two doesn't matter).
  void begin() {
    board_.begin();
    display_.begin();
    surface_.begin();
    gestures_.onGesture = &App::onGestureTrampoline;
    gestures_.isDraggable = &App::isDraggableTrampoline;
  }

  /// Call once per loop(): polls touch input, advances widget animations,
  /// and redraws only if something actually changed - see
  /// Screen::isDirty().
  void update() {
    gestures_.update(*board_.touch());
    screen_.update(millis());
    if (screen_.isDirty()) {
      screen_.draw(surface_);
      display_.writeData(surface_);
    }
  }

  /// Register widgets here - addWidget()/addFixedWidget()/presentDialog(),
  /// same as any other sketch's `screen`.
  Screen<RGB_T>& screen() { return screen_; }

  /// The theme in effect - see Screen::theme(). Handy for reading a color
  /// role directly (e.g. for a widget's own setColorOverride()) without
  /// keeping a separate MaterialTheme alongside `App`.
  const MaterialTheme<RGB_T>& theme() const { return screen_.theme(); }

  /// Rarely needed directly (App's own update() already draws to it) -
  /// exposed for the odd sketch that wants to draw something outside the
  /// widget tree, or call drawDirect()-style board APIs itself.
  tinygpu::Surface<RGB_T>& surface() { return surface_; }

  /// The display's width/height in pixels - straight from the board (see
  /// its own constructor), so a sketch never needs to declare its own
  /// `kWidth`/`kHeight` just to lay out widgets; use `app.width()`/
  /// `app.height()` directly wherever a widget's `Bounds` needs them.
  size_t width() const { return board_.width(); }
  size_t height() const { return board_.height(); }

 private:
  static void onGestureTrampoline(tinygpu::GestureEvent& event) { instance_->screen_.handleGesture(event); }
  static bool isDraggableTrampoline(int16_t x, int16_t y) {
    return instance_->screen_.isDraggableAt(x, y);
  }

  static App<RGB_T>* instance_;

  tinygpu::LCDBoard& board_;
  tinygpu::Surface<RGB_T> surface_;
  tinygpu::DeviceOutput<RGB_T> display_;
  tinygpu::GestureDetector gestures_;
  Screen<RGB_T> screen_;
};

template <typename RGB_T>
App<RGB_T>* App<RGB_T>::instance_ = nullptr;

}  // namespace tinymd
