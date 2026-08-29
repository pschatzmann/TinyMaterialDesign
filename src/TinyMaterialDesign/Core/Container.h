#pragma once
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "TinyGPU/Input/GestureDetector.h"
#include "TinyMaterialDesign/Core/Widget.h"

namespace tinymd {

/**
 * @brief A widget that holds child widgets, draws/dispatches gestures to
 * them, and scrolls its content vertically when it overflows its own bounds.
 *
 * This is the nestable counterpart to Screen (which does the same job once,
 * for the one root every sketch already has) - unlike GridLayout/LinearLayout/
 * etc. (pure rect calculators consumed before construction), Container is a
 * real Widget: it owns a child list, and since it's a Widget itself it can be
 * added as a child of another Container, nesting arbitrarily deep.
 *
 *   Container<RGB565> panel(Bounds(16, 56, 288, 300));
 *   panel.addChild(card1);
 *   panel.addChild(card2);   // more content than panel.bounds.h -> scrolls
 *   screen.addWidget(panel);
 *
 * Children are non-owning references (same convention as every other
 * composite in this library - Dialog's actions, Drawer's items, ...) and are
 * authored with absolute Bounds starting at/near this container's own
 * bounds.y, exactly like Screen's own scrollable content: at scrollOffset 0
 * a child renders at its own authored position, and scrolling shifts that
 * the same way Screen::draw() does.
 *
 * Children partially or fully scrolled past this container's own bounds
 * are cropped to it (see ISurface::pushClipRect()), not just skipped or
 * drawn in full - matching Screen's own root-level clip.
 *
 * Known limitation (shared with Screen - see its class comment): scrolling
 * is vertical only, one axis.
 *
 * Alternative to addChild(): setChildProvider(count, at) switches this
 * container to callback-driven content, for lists too large to keep every
 * item's Widget resident at once (thousands of rows) - `count` reports how
 * many logical items there are, `at(index)` returns a *reference* to the
 * Widget representing item `index`, typically a small pool of real Widgets
 * reused/repositioned/relabeled for whichever indices are currently visible
 * (the classic "recycler" pattern), rather than one Widget per item. Once a
 * provider is set it takes over completely - addChild()'d children are
 * ignored while one is active. Caution: a continuous gesture (drag/scroll)
 * latches the Widget* returned by `at()` at kBegan and keeps calling
 * methods on it through kChanged/kEnded - don't let the pool reassign that
 * same slot to a different index while a gesture is still in progress.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Container : public Widget<RGB_T> {
 public:
  using Widget<RGB_T>::bounds;
  using ChildCountFn = std::function<int()>;
  using ChildAtFn = std::function<Widget<RGB_T>&(int index)>;

  Container() = default;
  explicit Container(Bounds containerBounds) { bounds = containerBounds; }

  /// Registers `child` as content of this container. Not owned - see the
  /// class comment. `child` may itself be a Container. Ignored while a
  /// child provider is active - see setChildProvider().
  void addChild(Widget<RGB_T>& child) {
    children_.push_back(&child);
    if (this->theme_ != nullptr) child.setTheme(*this->theme_);
  }

  /// Switches this container to callback-driven content - see the class
  /// comment. Pass empty functions (the default-constructed state) to
  /// switch back to addChild()'s stored-vector mode.
  void setChildProvider(ChildCountFn count, ChildAtFn at) {
    childCountFn_ = std::move(count);
    childAtFn_ = std::move(at);
  }

  /// Fast path for contentHeight(): when every item is exactly `height`
  /// tall (uniform-height rows, the common case for a huge provider-backed
  /// list), content height becomes `effectiveCount() * height` - an O(1)
  /// calculation - instead of fetching and measuring every single item
  /// every frame (see contentHeight()'s own doc comment on why that
  /// matters at large counts). Pass 0 (the default) to go back to
  /// measuring each item; irrelevant in addChild() vector mode, where
  /// items are typically few enough that measuring them all is already
  /// cheap.
  void setUniformItemHeight(int32_t height) { uniformItemHeight_ = height; }

  /// Cascades the theme to every already-added child (vector mode). In
  /// provider mode, each Widget the provider hands back is themed at the
  /// point it's fetched instead (see effectiveChild()) - there's no fixed
  /// set of children to cascade to ahead of time.
  void setTheme(const MaterialTheme<RGB_T>& theme) override {
    Widget<RGB_T>::setTheme(theme);
    for (Widget<RGB_T>* child : children_) {
      if (child != nullptr) child->setTheme(theme);
    }
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    clampScroll();
    drawChildren(target);
    drawScrollbar(target);
  }

  /// Everything this container draws except its children - see
  /// Widget::drawBackground(). Just the scrollbar, since a Container has no
  /// chrome of its own.
  void drawBackground(tinygpu::ISurface<RGB_T>& target) override {
    clampScroll();
    drawScrollbar(target);
  }

  bool update(uint32_t nowMs) override {
    bool changed = false;
    const int count = effectiveCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* child = effectiveChild(i);
      if (child != nullptr) changed |= child->update(nowMs);
    }
    return changed;
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    using tinygpu::GesturePhase;
    using tinygpu::GestureType;

    if (isContinuousType(event.type)) {
      if (event.phase == GesturePhase::kBegan) {
        if (event.type == GestureType::kPan || event.type == GestureType::kScroll) {
          scrollDragActive_ = true;
          activeChild_ = nullptr;
        } else {
          scrollDragActive_ = false;
          activeChild_ = hitTest(event.startPoint.x, event.startPoint.y);
        }
      }

      bool handled = false;
      if (scrollDragActive_) {
        scrollOffset_ -= event.stepDeltaY;
        clampScroll();
        handled = true;
      } else if (activeChild_ != nullptr && activeChild_->enabled) {
        handled = dispatch(*activeChild_, event);
      }

      if (event.phase == GesturePhase::kEnded) {
        activeChild_ = nullptr;
        scrollDragActive_ = false;
      }
      return handled;
    }

    Widget<RGB_T>* hit = hitTest(event.point.x, event.point.y);
    if (hit != nullptr && hit->enabled) return dispatch(*hit, event);
    return false;
  }

  int childCount() const override { return effectiveCount(); }

  Widget<RGB_T>* child(int index) override { return effectiveChild(index); }

  /// Current vertical scroll offset (0 = content's natural top).
  int32_t scrollOffset() const { return scrollOffset_; }

  /// True if there's a child at (x, y) - this container's own coordinate
  /// space - that's draggable, recursing into it via its own
  /// isDraggableAt() if it's itself a composite (a nested Container,
  /// Dialog, ...) - see Widget::isDraggableAt(). Wire Screen::
  /// isDraggableAt() (or, for a Container nested inside another, this
  /// same method on the outer one) to this so a drag starting on it, at
  /// any nesting depth, is classified correctly.
  bool isDraggableAt(int32_t x, int32_t y) const override {
    Widget<RGB_T>* hit = hitTest(x, y);
    if (hit == nullptr) return false;
    return hit->isDraggableAt(x, y + scrollOffset_);
  }

  /// True for a gesture type that spans multiple events (kBegan/kChanged/
  /// kEnded) rather than firing once - used by callers that compose a
  /// Container (Screen; Drawer/BottomSheet/Menu's own item area) to decide
  /// whether to route a gesture here at all before it's mid-flight.
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

  bool isContainer() const override { return true; }

 protected:
  /// Height of this container's content, measured from its own bounds.y -
  /// mirrors Screen::contentHeight() but relative to this container's own
  /// top rather than the whole surface. Protected: Screen reuses this
  /// directly for its own drawDirect()/drawScrollbarDirect(), since it's a
  /// Container itself and drives the same scroll math one layer further out.
  /// In provider mode this visits every logical index to find the tallest -
  /// still O(count) rather than O(memory), but a huge count makes this (and
  /// so every clampScroll() call, i.e. every frame) proportionally slower;
  /// keep item heights cheap to compute if count is large, or call
  /// setUniformItemHeight() for an O(1) alternative when rows are all the
  /// same height.
  int32_t contentHeight() const {
    const int count = effectiveCount();
    if (uniformItemHeight_ > 0) return count * uniformItemHeight_;
    int32_t bottom = bounds.y;
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* child = effectiveChild(i);
      if (child != nullptr) bottom = std::max(bottom, child->bounds.bottom());
    }
    return bottom - bounds.y;
  }

  void clampScroll() {
    const int32_t maxScroll = std::max(int32_t{0}, contentHeight() - bounds.h);
    scrollOffset_ = std::min(std::max(scrollOffset_, int32_t{0}), maxScroll);
  }

  /// Number of children right now - the provider's count() if
  /// setChildProvider() is active, else the addChild()'d vector's size.
  /// Protected: Screen's drawDirect() drives the same loop one layer out.
  int effectiveCount() const {
    return childCountFn_ ? childCountFn_() : static_cast<int>(children_.size());
  }

  /// The child at `index` right now - from the provider if active (themed
  /// immediately, since there's no fixed set to cascade setTheme() to ahead
  /// of time - see setChildProvider()), else from the addChild()'d vector.
  /// Protected: Screen's drawDirect() drives the same loop one layer out.
  Widget<RGB_T>* effectiveChild(int index) const {
    if (childAtFn_) {
      Widget<RGB_T>& child = childAtFn_(index);
      if (this->theme_ != nullptr) child.setTheme(*this->theme_);
      return &child;
    }
    return children_[static_cast<size_t>(index)];
  }

  std::vector<Widget<RGB_T>*> children_;
  ChildCountFn childCountFn_;
  ChildAtFn childAtFn_;
  int32_t uniformItemHeight_ = 0;
  Widget<RGB_T>* activeChild_ = nullptr;
  bool scrollDragActive_ = false;
  int32_t scrollOffset_ = 0;

 private:
  /// True if screen-space `b` overlaps this container's own bounds at all -
  /// a cheap pre-filter so a child scrolled entirely out of view is skipped
  /// without even being drawn (drawChildren()'s clip rect handles correctly
  /// cropping the ones that only partially overlap).
  bool intersectsBounds(const Bounds& b) const {
    return b.bottom() > bounds.y && b.y < bounds.bottom() && b.right() > bounds.x && b.x < bounds.right();
  }

  /// Draws every visible child, each cropped to this container's own
  /// bounds (see ISurface::pushClipRect()) - a child only partially
  /// overlapping (scrolled halfway past the top/bottom edge) is cropped
  /// to the visible portion instead of drawn in full or skipped outright.
  void drawChildren(tinygpu::ISurface<RGB_T>& target) {
    target.pushClipRect(toPx(bounds.x), toPx(bounds.y), toPx(bounds.w), toPx(bounds.h));
    const int count = effectiveCount();
    for (int i = 0; i < count; ++i) {
      Widget<RGB_T>* child = effectiveChild(i);
      if (child == nullptr || !child->visible) continue;
      child->bounds.y -= scrollOffset_;
      if (intersectsBounds(child->bounds)) child->draw(target);
      child->bounds.y += scrollOffset_;
    }
    target.popClipRect();
  }

  /// A thin right-edge indicator of scroll position/range, confined to this
  /// container's own bounds - mirrors Screen::drawScrollbar(). Omitted
  /// entirely once content already fits.
  void drawScrollbar(tinygpu::ISurface<RGB_T>& target) {
    const int32_t content = contentHeight();
    if (content <= bounds.h) return;

    constexpr int32_t kBarWidth = 4;
    const int32_t trackX = bounds.right() - kBarWidth;
    target.fillRect(toPx(trackX), toPx(bounds.y), toPx(kBarWidth), toPx(bounds.h),
                    blend(this->theme().colors.surface, this->theme().colors.outline, 0.25f));

    const int32_t maxScroll = content - bounds.h;
    const int32_t thumbHeight = std::max(int32_t(24), bounds.h * bounds.h / content);
    const int32_t thumbY =
        maxScroll > 0 ? bounds.y + (scrollOffset_ * (bounds.h - thumbHeight)) / maxScroll : bounds.y;
    target.fillRoundRect(toPx(trackX), toPx(thumbY), toPx(kBarWidth), toPx(thumbHeight), toPx(kBarWidth / 2),
                         this->theme().colors.primary);
  }

  /// Last-added child wins ties, i.e. later addChild() calls are treated as
  /// drawn on top (matches drawChildren()). (x, y) is this container's own
  /// coordinate space; translated into content space by the current scroll
  /// offset before testing, since child bounds are authored in content
  /// space (see the class comment).
  Widget<RGB_T>* hitTest(int32_t x, int32_t y) const {
    const int32_t contentY = y + scrollOffset_;
    for (int i = effectiveCount() - 1; i >= 0; --i) {
      Widget<RGB_T>* child = effectiveChild(i);
      if (child != nullptr && child->visible && child->enabled && child->bounds.contains(x, contentY)) {
        return child;
      }
    }
    return nullptr;
  }

  /// Forwards `event` to `child`, translating its point/startPoint back into
  /// content space first - mirrors Screen::dispatch().
  bool dispatch(Widget<RGB_T>& child, const tinygpu::GestureEvent& event) {
    if (scrollOffset_ == 0) return child.onGesture(event);
    tinygpu::GestureEvent shifted = event;
    shifted.point.y = static_cast<int16_t>(shifted.point.y + scrollOffset_);
    shifted.startPoint.y = static_cast<int16_t>(shifted.startPoint.y + scrollOffset_);
    return child.onGesture(shifted);
  }
};

}  // namespace tinymd
