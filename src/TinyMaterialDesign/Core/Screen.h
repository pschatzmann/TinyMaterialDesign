#pragma once
#include <algorithm>
#include <vector>

#include "TinyGPU/Drivers/DisplayDriver.h"
#include "TinyGPU/Input/GestureDetector.h"
#include "TinyGPU/Surface/WindowedSurface.h"
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
 *   Screen<RGB565> screen(theme);
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
 *     if (screen.isDirty()) {          // see isDirty() - skips the (slow)
 *       screen.draw(surface);          // redraw + display write when
 *       display.writeData(surface);    // nothing changed this frame
 *     }
 *   }
 *
 * Vertical scrolling: widgets added via addWidget() are laid out in one
 * "content" coordinate space that can be taller than the surface passed to
 * draw() - if it is, dragging up/down over any non-interactive spot (an
 * empty gap, a Label, a Card, ...) scrolls that content, the same gesture
 * TouchDriver/GestureDetector already classify as kScroll/kPan. Widgets
 * that should stay put while the content behind them scrolls - a top
 * AppBar, a bottom Keyboard - go through addFixedWidget() instead; they're
 * always drawn and hit-tested on top of the scrollable content, at their
 * own authored (screen-space) bounds. If everything already fits within
 * the drawn surface, scrolling never engages (offset stays 0), no
 * scrollbar is drawn, and existing sketches that only call addWidget()
 * are unaffected. A thin scrollbar on the right edge appears automatically
 * whenever there's something to scroll - see drawScrollbar().
 *
 * Known limitation: there is no sub-widget clip rect anywhere in this
 * library (see e.g. TextArea's own overflow caveat), so a widget that
 * straddles the top edge of the viewport while scrolled partway off it
 * renders pinned to y=0 at full size for that moment rather than being
 * cropped - cosmetic only, and gone once it's fully scrolled past.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Screen {
 public:
  Screen() = default;

  /// Constructs a Screen that starts with `theme` already set - pushed into
  /// every widget registered afterward (see setTheme()).
  explicit Screen(const MaterialTheme<RGB_T>& theme) : theme_(theme) {}

  /// Registers `widget` as scrollable content (see the class comment).
  void addWidget(Widget<RGB_T>& widget) {
    scrollWidgets_.push_back(&widget);
    widget.setTheme(theme_);
    dirty_ = true;
  }

  /// Registers `widget` pinned to its authored bounds regardless of
  /// scrolling - e.g. a top AppBar or bottom Keyboard. Always drawn and
  /// hit-tested on top of every scrollable widget.
  void addFixedWidget(Widget<RGB_T>& widget) {
    fixedWidgets_.push_back(&widget);
    widget.setTheme(theme_);
    dirty_ = true;
  }

  /// Replaces the screen's theme and pushes it into every widget already
  /// registered (scrollable, fixed, and a presented dialog if any).
  /// Widgets registered later (addWidget()/addFixedWidget()/
  /// presentDialog()) pick up whatever theme is current at that time.
  void setTheme(const MaterialTheme<RGB_T>& theme) {
    theme_ = theme;
    for (Widget<RGB_T>* w : scrollWidgets_) if (w != nullptr) w->setTheme(theme_);
    for (Widget<RGB_T>* w : fixedWidgets_) if (w != nullptr) w->setTheme(theme_);
    if (dialog_ != nullptr) dialog_->setTheme(theme_);
    dirty_ = true;
  }

  /// Current vertical scroll offset (0 = content's natural top).
  int32_t scrollOffset() const { return scrollOffset_; }

  /// Overrides the theme's background color for this screen. Not required -
  /// draw() falls back to theme_.colors.background.
  void setBackgroundColor(RGB_T color) {
    backgroundColor_ = color;
    hasBackgroundColor_ = true;
    dirty_ = true;
  }

  /// True if draw() would paint something different than last time - see
  /// invalidate(). A typical loop() should skip draw()/writeData() while
  /// this is false:
  ///
  ///   screen.update(millis());
  ///   if (screen.isDirty()) {
  ///     screen.draw(surface);
  ///     display.writeData(surface);
  ///   }
  ///
  /// Starts true so the first frame always draws. Set whenever a gesture
  /// reaches handleGesture() (tap/drag/scroll - covers ripple starts,
  /// toggles, slider drags, and anything a click/change callback does to
  /// this or any other widget, since those callbacks run synchronously
  /// during dispatch) or when update() advances a time-based animation
  /// (ripple fade, indeterminate progress sweep, cursor blink). Cleared
  /// once draw() has run.
  ///
  /// This is a coarse, screen-wide flag, not per-widget dirty rects - one
  /// changed widget still repaints the whole screen. It also can't see
  /// state a sketch mutates directly on a widget from outside a gesture/
  /// update callback (e.g. a Label updated from a sensor reading in
  /// loop()) - call invalidate() explicitly after changes like that.
  bool isDirty() const { return dirty_; }

  /// Forces the next isDirty() to be true / the next draw() to run, for
  /// widget state changed from outside a gesture or update() callback -
  /// see isDirty().
  void invalidate() { dirty_ = true; }

  void draw(tinygpu::ISurface<RGB_T>& target) {
    viewportHeight_ = static_cast<int32_t>(target.height());
    clampScroll();

    target.clear(hasBackgroundColor_ ? backgroundColor_ : theme_.colors.background);
    for (Widget<RGB_T>* widget : scrollWidgets_) {
      if (!widget->visible) continue;
      widget->bounds.y -= scrollOffset_;
      widget->draw(target);
      widget->bounds.y += scrollOffset_;
    }
    drawScrollbar(target);
    for (Widget<RGB_T>* widget : fixedWidgets_) {
      if (widget->visible) widget->draw(target);
    }
    if (dialog_ != nullptr && dialog_->visible) {
      dialog_->draw(target);
    }
    dirty_ = false;
  }

  /// Renders the full layout straight to `driver` without ever needing a
  /// full-screen framebuffer - for boards that can't spare a single
  /// contiguous allocation the size of the whole screen (e.g. classic
  /// ESP32 without PSRAM: a 240x320 RGB565 buffer is 153,600 bytes, which
  /// can exceed the largest free block even with plenty of *total* free
  /// heap, since the heap ends up split into multiple smaller regions).
  ///
  /// Each visible widget - scrollable or fixed, skipping anything
  /// invisible or entirely outside the viewport (including scrolled off
  /// it), same idea as content that's never drawn today - gets `scratch`
  /// resized to exactly its own bounds, is drawn into it once, and is
  /// blitted directly to the panel at its own position: genuinely one
  /// small buffer and one write per control, not a shared full-screen (or
  /// full-band) one. `scratch` is one small, reused, real
  /// (non-owning, already-begin()'d) surface - since Surface::resize()
  /// only reallocates when growing past its current capacity, repeatedly
  /// resizing it this way settles down to zero reallocation once it's
  /// been sized to the largest single widget this layout ever draws.
  ///
  /// The one case that can't work this way is a presented modal
  /// (Dialog/Drawer): its scrim always covers the *full* viewport
  /// regardless of the modal's own bounds (see Dialog.h/Drawer.h), and it
  /// can itself span most of the screen (a full-height Drawer) - both too
  /// much for one small per-widget buffer. So while a modal is presented,
  /// everything else is skipped outright (it would just be fully
  /// occluded) and only the modal is drawn: its drawBackground() (see
  /// Widget.h) - scrim plus whatever chrome isn't one of its children -
  /// one short horizontal strip at a time via a WindowedSurface (see that
  /// class) that reports the full viewport size for the scrim's own math
  /// but clips actual pixel writes to the current strip, followed by each
  /// of its children (see childCount()/child()) individually via the same
  /// per-widget path as everything else - e.g. Drawer's own list items,
  /// each drawn once rather than redrawn on every strip.  A modal type
  /// that doesn't override drawBackground()/childCount() (Dialog, as yet)
  /// falls back to the default drawBackground() = draw() and childCount()
  /// = 0, i.e. its *entire* contents (chrome + actions) drawn banded, same
  /// as before.
  ///
  /// This is an alternative to draw()+writeData(surface) (unaffected by
  /// this method's existence) - pick whichever fits your board.
  void drawDirect(tinygpu::DisplayDriver<RGB_T>& driver, tinygpu::ISurface<RGB_T>& scratch,
                  int32_t viewportWidth, int32_t viewportHeight) {
    viewportHeight_ = viewportHeight;
    clampScroll();

    if (dialog_ != nullptr && dialog_->visible) {
      drawModalDirect(driver, scratch, viewportWidth, viewportHeight);
      dirty_ = false;
      return;
    }

    const RGB_T background = hasBackgroundColor_ ? backgroundColor_ : theme_.colors.background;

    // Unlike draw() (which clears the whole target surface up front - see
    // its target.clear() call above), per-widget rendering only ever
    // touches each widget's own small bounds - nothing paints the gaps
    // between/around them otherwise, which would just show whatever was
    // on the panel before (stale content, or black on first boot).
    // writeColor() is DisplayDriver's own cheap one-row-buffer solid
    // fill (see its doc comment) - no full-viewport buffer needed here
    // either.
    driver.writeColor(static_cast<size_t>(viewportWidth), static_cast<size_t>(viewportHeight),
                      background);

    for (Widget<RGB_T>* widget : scrollWidgets_) {
      if (!widget->visible) continue;
      Bounds screenBounds = widget->bounds;
      screenBounds.y -= scrollOffset_;
      if (!intersectsViewport(screenBounds, viewportWidth, viewportHeight)) continue;
      widget->bounds.y -= scrollOffset_;
      drawWidgetDirect(driver, scratch, *widget, background);
      widget->bounds.y += scrollOffset_;
    }
    drawScrollbarDirect(driver, scratch, viewportWidth, viewportHeight);
    for (Widget<RGB_T>* widget : fixedWidgets_) {
      if (!widget->visible) continue;
      if (!intersectsViewport(widget->bounds, viewportWidth, viewportHeight)) continue;
      drawWidgetDirect(driver, scratch, *widget, background);
    }
    dirty_ = false;
  }

  /// Advances any time-based widget animation. Call once per loop(), before
  /// checking isDirty().
  void update(uint32_t nowMs) {
    bool changed = false;
    for (Widget<RGB_T>* widget : scrollWidgets_) changed |= widget->update(nowMs);
    for (Widget<RGB_T>* widget : fixedWidgets_) changed |= widget->update(nowMs);
    if (dialog_ != nullptr) changed |= dialog_->update(nowMs);
    if (changed) dirty_ = true;
  }

  /// Wire this to GestureDetector::isDraggable so a drag starting on a
  /// draggable widget (currently only Slider) is classified as kDrag
  /// instead of kPan/kScroll.
  bool isDraggableAt(int32_t x, int32_t y) const {
    if (dialog_ != nullptr) return false;
    const HitResult hit = hitTestDetailed(x, y);
    return hit.widget != nullptr && hit.widget->isDraggable();
  }

  /// Wire this to GestureDetector::onGesture.
  void handleGesture(const tinygpu::GestureEvent& event) {
    using tinygpu::GesturePhase;
    using tinygpu::GestureType;

    // Any gesture reaching here might change something on screen (a ripple
    // starting, a drag position, a scroll offset, or anything a click/
    // change callback touches) - see isDirty(). Coarse but cheap, and only
    // as frequent as actual input, not the loop() rate.
    dirty_ = true;

    if (dialog_ != nullptr) {
      // Modal: every gesture goes to the presented dialog while it's shown.
      // Dialog bounds are always screen-space, so no offset applies.
      if (dialog_->enabled) dialog_->onGesture(event);
      if (isContinuousType(event.type) && event.phase == GesturePhase::kEnded) {
        activeWidget_ = nullptr;
        scrollDragActive_ = false;
      }
      return;
    }

    if (isContinuousType(event.type)) {
      // Drag-like gestures: latch what kBegan hit and keep routing to it
      // for kChanged/kEnded even if the pointer leaves its bounds -
      // otherwise dragging a Slider thumb past its own bounds would stop
      // updating it. kPan/kScroll only ever fire over a non-draggable
      // start point (see GestureDetector::classifyDragType), so they
      // always mean "scroll the content" here rather than routing to
      // whatever passive widget (if any) happens to sit under them.
      if (event.phase == GesturePhase::kBegan) {
        if (event.type == GestureType::kPan || event.type == GestureType::kScroll) {
          scrollDragActive_ = true;
          activeWidget_ = nullptr;
        } else {
          scrollDragActive_ = false;
          const HitResult hit = hitTestDetailed(event.startPoint.x, event.startPoint.y);
          activeWidget_ = hit.widget;
          activeWidgetScrollable_ = hit.scrollable;
        }
      }

      if (scrollDragActive_) {
        scrollOffset_ -= event.stepDeltaY;
        clampScroll();
      } else if (activeWidget_ != nullptr && activeWidget_->enabled) {
        dispatch(*activeWidget_, event, activeWidgetScrollable_);
      }

      if (event.phase == GesturePhase::kEnded) {
        activeWidget_ = nullptr;
        scrollDragActive_ = false;
      }
      return;
    }

    // Discrete gestures (tap/double-tap/long-press/swipe-*) always report
    // phase kEnded with no preceding kBegan, so they're hit-tested fresh
    // at the event's own point rather than routed through activeWidget_.
    const HitResult hit = hitTestDetailed(event.point.x, event.point.y);
    if (hit.widget != nullptr && hit.widget->enabled) {
      dispatch(*hit.widget, event, hit.scrollable);
    }
  }

  /// Shows `dialog` modally: it receives every gesture and is drawn on top
  /// until dismissDialog() is called (typically from one of the dialog's
  /// own action-button callbacks).
  void presentDialog(Widget<RGB_T>& dialog) {
    dialog_ = &dialog;
    dialog.setTheme(theme_);
    dirty_ = true;
  }
  void dismissDialog() {
    dialog_ = nullptr;
    dirty_ = true;
  }
  bool isDialogPresented() const { return dialog_ != nullptr; }

 private:
  /// True if screen-space `bounds` overlaps the viewport at all - see
  /// drawDirect().
  static bool intersectsViewport(const Bounds& bounds, int32_t viewportWidth, int32_t viewportHeight) {
    return bounds.bottom() > 0 && bounds.y < viewportHeight && bounds.right() > 0 &&
           bounds.x < viewportWidth;
  }

  /// Draws one widget into `scratch` (resized to exactly its own bounds)
  /// and blits it to `driver` at its own position - see drawDirect().
  /// No-ops (leaving the display unwritten) if `widget`'s bounds are
  /// empty or scratch.resize() fails.
  void drawWidgetDirect(tinygpu::DisplayDriver<RGB_T>& driver, tinygpu::ISurface<RGB_T>& scratch,
                        Widget<RGB_T>& widget, RGB_T background) {
    const int32_t x = widget.bounds.x;
    const int32_t y = widget.bounds.y;
    const int32_t w = widget.bounds.w;
    const int32_t h = widget.bounds.h;
    if (w <= 0 || h <= 0) return;
    if (!scratch.resize(static_cast<size_t>(w), static_cast<size_t>(h))) return;

    tinygpu::WindowedSurface<RGB_T> window(scratch, x, y, static_cast<size_t>(w),
                                           static_cast<size_t>(h), scratch.font());
    window.clear(background);
    widget.draw(window);
    driver.writeData(scratch, x, y);
  }

  /// Draws the right-edge scroll position indicator directly - mirrors
  /// drawScrollbar() (the draw()-path version) exactly, just resized/
  /// blitted instead of drawn into the shared full-size target. Only
  /// kBarWidth px wide regardless of viewport height, so - unlike a modal -
  /// this always fits in one shot.
  void drawScrollbarDirect(tinygpu::DisplayDriver<RGB_T>& driver, tinygpu::ISurface<RGB_T>& scratch,
                           int32_t viewportWidth, int32_t viewportHeight) {
    const int32_t content = contentHeight();
    if (content <= viewportHeight) return;

    constexpr int32_t kBarWidth = 4;
    const int32_t trackX = viewportWidth - kBarWidth;
    if (!scratch.resize(static_cast<size_t>(kBarWidth), static_cast<size_t>(viewportHeight))) return;

    tinygpu::WindowedSurface<RGB_T> window(scratch, trackX, 0, static_cast<size_t>(kBarWidth),
                                           static_cast<size_t>(viewportHeight), scratch.font());
    window.fillRect(trackX, 0, kBarWidth, viewportHeight,
                    blend(theme_.colors.surface, theme_.colors.outline, 0.25f));

    const int32_t maxScroll = content - viewportHeight;
    const int32_t thumbHeight =
        std::max(int32_t(24), viewportHeight * viewportHeight / content);
    const int32_t thumbY = maxScroll > 0
                               ? (scrollOffset_ * (viewportHeight - thumbHeight)) / maxScroll
                               : 0;
    window.fillRoundRect(trackX, thumbY, kBarWidth, thumbHeight, kBarWidth / 2, theme_.colors.primary);
    driver.writeData(scratch, trackX, 0);
  }

  /// Draws the presented modal - its drawBackground() banded (see
  /// drawDirect() for why), then each of its children (if any)
  /// individually via drawWidgetDirect(). `scratch` ends this method
  /// resized to `viewportWidth x kModalBandHeight`.
  void drawModalDirect(tinygpu::DisplayDriver<RGB_T>& driver, tinygpu::ISurface<RGB_T>& scratch,
                       int32_t viewportWidth, int32_t viewportHeight) {
    const RGB_T background = hasBackgroundColor_ ? backgroundColor_ : theme_.colors.background;

    constexpr int32_t kModalBandHeight = 40;
    if (scratch.resize(static_cast<size_t>(viewportWidth), static_cast<size_t>(kModalBandHeight))) {
      for (int32_t bandY = 0; bandY < viewportHeight; bandY += kModalBandHeight) {
        tinygpu::WindowedSurface<RGB_T> window(scratch, 0, bandY,
                                               static_cast<size_t>(viewportWidth),
                                               static_cast<size_t>(viewportHeight),
                                               scratch.font());
        // drawBackground() darkens whatever's already in `window` (see
        // Widget::drawScrim()) rather than painting a flat scrim color -
        // deliberately just a clean background here, not the real
        // regular content, on this memory/CPU-constrained rendering
        // path (see drawDirect()'s doc comment) - without this clear()
        // it would darken whatever stale pixels `scratch` happened to
        // still hold from the last (differently-sized) widget drawn
        // into it.
        window.clear(background);
        dialog_->drawBackground(window);
        driver.writeData(scratch, 0, bandY);
      }
    }

    const int childCount = dialog_->childCount();
    for (int i = 0; i < childCount; ++i) {
      Widget<RGB_T>* child = dialog_->child(i);
      if (child != nullptr && child->visible) {
        drawWidgetDirect(driver, scratch, *child, background);
      }
    }
  }

  std::vector<Widget<RGB_T>*> scrollWidgets_;
  std::vector<Widget<RGB_T>*> fixedWidgets_;
  Widget<RGB_T>* activeWidget_ = nullptr;
  bool activeWidgetScrollable_ = false;
  bool scrollDragActive_ = false;
  int32_t scrollOffset_ = 0;
  int32_t viewportHeight_ = 0;
  bool dirty_ = true;
  Widget<RGB_T>* dialog_ = nullptr;
  RGB_T backgroundColor_{};
  bool hasBackgroundColor_ = false;
  MaterialTheme<RGB_T> theme_{};

  struct HitResult {
    Widget<RGB_T>* widget;
    bool scrollable;
  };

  /// Last-added widget wins ties within each group, i.e. later addWidget()/
  /// addFixedWidget() calls are treated as drawn on top (matches draw()).
  /// Fixed widgets are checked first since they're always drawn over
  /// scrollable ones; (x, y) is screen-space either way - scrollable
  /// bounds are authored in content space, so the point is translated by
  /// the current scroll offset only for that half of the search.
  HitResult hitTestDetailed(int32_t x, int32_t y) const {
    for (auto it = fixedWidgets_.rbegin(); it != fixedWidgets_.rend(); ++it) {
      Widget<RGB_T>* widget = *it;
      if (widget->visible && widget->enabled && widget->bounds.contains(x, y)) {
        return {widget, false};
      }
    }
    const int32_t contentY = y + scrollOffset_;
    for (auto it = scrollWidgets_.rbegin(); it != scrollWidgets_.rend(); ++it) {
      Widget<RGB_T>* widget = *it;
      if (widget->visible && widget->enabled && widget->bounds.contains(x, contentY)) {
        return {widget, true};
      }
    }
    return {nullptr, false};
  }

  /// Forwards `event` to `widget`, translating its point/startPoint back
  /// into content space first if `widget` is one of the scrollable ones -
  /// their bounds (and so their own hit-testing, e.g. Keyboard's per-key
  /// rects) are authored in content space, not screen space.
  void dispatch(Widget<RGB_T>& widget, const tinygpu::GestureEvent& event, bool scrollable) {
    if (!scrollable || scrollOffset_ == 0) {
      widget.onGesture(event);
      return;
    }
    tinygpu::GestureEvent shifted = event;
    shifted.point.y = static_cast<int16_t>(shifted.point.y + scrollOffset_);
    shifted.startPoint.y = static_cast<int16_t>(shifted.startPoint.y + scrollOffset_);
    widget.onGesture(shifted);
  }

  int32_t contentHeight() const {
    int32_t bottom = 0;
    for (Widget<RGB_T>* widget : scrollWidgets_) bottom = std::max(bottom, widget->bounds.bottom());
    return bottom;
  }

  void clampScroll() {
    const int32_t maxScroll = std::max(int32_t{0}, contentHeight() - viewportHeight_);
    scrollOffset_ = std::min(std::max(scrollOffset_, int32_t{0}), maxScroll);
  }

  /// A thin right-edge indicator of scroll position/range - the only visual
  /// cue that there's more content than fits, since nothing else about a
  /// scrollable Screen looks different from one that isn't. Drawn between
  /// the scrollable content and the fixed widgets, so a pinned AppBar/
  /// Keyboard naturally occludes it where they overlap (top/bottom), and
  /// only omitted entirely when content already fits (nothing to scroll).
  void drawScrollbar(tinygpu::ISurface<RGB_T>& target) {
    const int32_t content = contentHeight();
    if (content <= viewportHeight_) return;

    constexpr int32_t kBarWidth = 4;
    const int32_t trackX = static_cast<int32_t>(target.width()) - kBarWidth;
    target.fillRect(toPx(trackX), 0, toPx(kBarWidth), toPx(viewportHeight_),
                    blend(theme_.colors.surface, theme_.colors.outline, 0.25f));

    const int32_t maxScroll = content - viewportHeight_;
    const int32_t thumbHeight =
        std::max(int32_t(24), viewportHeight_ * viewportHeight_ / content);
    const int32_t thumbY = maxScroll > 0
                               ? (scrollOffset_ * (viewportHeight_ - thumbHeight)) / maxScroll
                               : 0;
    target.fillRoundRect(toPx(trackX), toPx(thumbY), toPx(kBarWidth), toPx(thumbHeight),
                         toPx(kBarWidth / 2), theme_.colors.primary);
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
