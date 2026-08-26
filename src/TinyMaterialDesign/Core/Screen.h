#pragma once
#include <vector>

#include "TinyGPU/Input/GestureDetector.h"
#include "TinyMaterialDesignConfig.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief Owns a set of widgets, draws them, and routes gesture events to
 * whichever one should handle each event.
 *
 * Screen does not own the widgets (the sketch does, typically as globals -
 * the same non-owning-reference composition TinyGPU itself uses everywhere,
 * e.g. DeviceOutput(driver&)). Usage, mirroring TinyGPU's own examples:
 *
 *   Screen<RGB565> screen;
 *   Button<RGB565> okButton(...);
 *   screen.addWidget(okButton);
 *
 *   GestureDetector gestures;
 *   gestures.onGesture = [](GestureEvent& e) { screen.handleGesture(e); };
 *   gestures.isDraggable = [](int16_t x, int16_t y) {
 *     return screen.isDraggableAt(x, y);
 *   };
 *
 *   void loop() {
 *     gestures.update(touchDriver);
 *     screen.update(millis());
 *     screen.draw(surface, theme);
 *     display.writeData(surface);
 *   }
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Screen {
 public:
  void addWidget(Widget<RGB_T>& widget) { widgets_.push_back(&widget); }

  /// Overrides the theme's background color for this screen. Not required -
  /// draw() falls back to theme.colors.background.
  void setBackgroundColor(RGB_T color) {
    backgroundColor_ = color;
    hasBackgroundColor_ = true;
  }

  void draw(tinygpu::ISurface<RGB_T>& target, const MaterialTheme<RGB_T>& theme) {
    target.clear(hasBackgroundColor_ ? backgroundColor_ : theme.colors.background);
    for (Widget<RGB_T>* widget : widgets_) {
      if (widget->visible) widget->draw(target, theme);
    }
    if (dialog_ != nullptr && dialog_->visible) {
      dialog_->draw(target, theme);
    }
  }

  /// Advances any time-based widget animation. Call once per loop().
  void update(uint32_t nowMs) {
    for (Widget<RGB_T>* widget : widgets_) widget->update(nowMs);
    if (dialog_ != nullptr) dialog_->update(nowMs);
  }

  /// Wire this to GestureDetector::isDraggable so a drag starting on a
  /// draggable widget (currently only Slider) is classified as kDrag
  /// instead of kPan/kScroll.
  bool isDraggableAt(int32_t x, int32_t y) const {
    if (dialog_ != nullptr) return false;
    Widget<RGB_T>* widget = hitTest(x, y);
    return widget != nullptr && widget->isDraggable();
  }

  /// Wire this to GestureDetector::onGesture.
  void handleGesture(const tinygpu::GestureEvent& event) {
    using tinygpu::GesturePhase;

    Widget<RGB_T>* target = nullptr;

    if (dialog_ != nullptr) {
      // Modal: every gesture goes to the presented dialog while it's shown.
      target = dialog_;
    } else if (isContinuousType(event.type)) {
      // Drag-like gestures: latch the widget hit at kBegan and keep routing
      // to it for kChanged/kEnded even if the pointer leaves its bounds -
      // otherwise dragging a Slider thumb past its own bounds would stop
      // updating it.
      if (event.phase == GesturePhase::kBegan) {
        activeWidget_ = hitTest(event.startPoint.x, event.startPoint.y);
      }
      target = activeWidget_;
    } else {
      // Discrete gestures (tap/double-tap/long-press/swipe-*) always report
      // phase kEnded with no preceding kBegan, so they're hit-tested fresh
      // at the event's own point rather than routed through activeWidget_.
      target = hitTest(event.point.x, event.point.y);
    }

    if (target != nullptr && target->enabled) {
      target->onGesture(event);
    }

    if (isContinuousType(event.type) && event.phase == GesturePhase::kEnded) {
      activeWidget_ = nullptr;
    }
  }

  /// Shows `dialog` modally: it receives every gesture and is drawn on top
  /// until dismissDialog() is called (typically from one of the dialog's
  /// own action-button callbacks).
  void presentDialog(Widget<RGB_T>& dialog) { dialog_ = &dialog; }
  void dismissDialog() { dialog_ = nullptr; }
  bool isDialogPresented() const { return dialog_ != nullptr; }

 private:
  std::vector<Widget<RGB_T>*> widgets_;
  Widget<RGB_T>* activeWidget_ = nullptr;
  Widget<RGB_T>* dialog_ = nullptr;
  RGB_T backgroundColor_{};
  bool hasBackgroundColor_ = false;

  /// Last-added widget wins ties, i.e. later addWidget() calls are treated
  /// as drawn on top (matches draw() iterating widgets_ front-to-back).
  Widget<RGB_T>* hitTest(int32_t x, int32_t y) const {
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
      Widget<RGB_T>* widget = *it;
      if (widget->visible && widget->enabled && widget->bounds.contains(x, y)) {
        return widget;
      }
    }
    return nullptr;
  }

  static bool isContinuousType(tinygpu::GestureType type) {
    using tinygpu::GestureType;
    switch (type) {
      case GestureType::kDrag:
      case GestureType::kPan:
      case GestureType::kScroll:
      case GestureType::kPinchIn:
      case GestureType::kPinchOut:
      case GestureType::kRotate:
        return true;
      default:
        return false;
    }
  }
};

}  // namespace tinymd
