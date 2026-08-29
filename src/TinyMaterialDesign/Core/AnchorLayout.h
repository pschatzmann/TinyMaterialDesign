#pragma once
#include "TinyMaterialDesign/Core/Bounds.h"

namespace tinymd {

enum class Anchor {
  TopLeft,
  TopCenter,
  TopRight,
  CenterLeft,
  Center,
  CenterRight,
  BottomLeft,
  BottomCenter,
  BottomRight
};

/**
 * @brief Positions a single rect relative to a corner/edge of a container.
 *
 * Not a widget - see GridLayout.h for the rationale. Solves the "place a FAB
 * in the bottom-right" / "badge in the top-right corner of an avatar" cases
 * that would otherwise be hand-computed arithmetic in every sketch:
 *
 *   fab.bounds = AnchorLayout(screenBounds, 16).rect(Anchor::BottomRight, 56, 56);
 *   badge.bounds = AnchorLayout(avatar.bounds, 0).rect(Anchor::TopRight, 16, 16);
 */
class AnchorLayout {
 public:
  explicit AnchorLayout(Bounds container, int32_t margin = 0) : container_(container), margin_(margin) {}

  /// Bounds for a `width` x `height` rect anchored at `anchor`, inset by margin.
  Bounds rect(Anchor anchor, int32_t width, int32_t height) const {
    int32_t x;
    int32_t y;
    switch (anchor) {
      case Anchor::TopLeft:
        x = container_.x + margin_;
        y = container_.y + margin_;
        break;
      case Anchor::TopCenter:
        x = container_.centerX() - width / 2;
        y = container_.y + margin_;
        break;
      case Anchor::TopRight:
        x = container_.right() - margin_ - width;
        y = container_.y + margin_;
        break;
      case Anchor::CenterLeft:
        x = container_.x + margin_;
        y = container_.centerY() - height / 2;
        break;
      case Anchor::Center:
        x = container_.centerX() - width / 2;
        y = container_.centerY() - height / 2;
        break;
      case Anchor::CenterRight:
        x = container_.right() - margin_ - width;
        y = container_.centerY() - height / 2;
        break;
      case Anchor::BottomLeft:
        x = container_.x + margin_;
        y = container_.bottom() - margin_ - height;
        break;
      case Anchor::BottomCenter:
        x = container_.centerX() - width / 2;
        y = container_.bottom() - margin_ - height;
        break;
      case Anchor::BottomRight:
      default:
        x = container_.right() - margin_ - width;
        y = container_.bottom() - margin_ - height;
        break;
    }
    return Bounds(x, y, width, height);
  }

 private:
  Bounds container_;
  int32_t margin_;
};

}  // namespace tinymd
