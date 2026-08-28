#pragma once
#include <functional>
#include <vector>

#include "TinyGPU/Color/RGB565.h"
#include "TinyGPU/Color/RGB666.h"
#include "TinyGPU/Color/RGB888.h"
#include "TinyMaterialDesign/Core/Widget.h"
#include "TinyMaterialDesign/Draw/Icons.h"

namespace tinymd {

/// One radio button. Tapping it selects it (radio semantics: a tap never
/// deselects); mutual exclusion across a group is RadioGroup's job.
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class RadioButton : public Widget<RGB_T> {
 public:
  RadioButton() = default;
  explicit RadioButton(Bounds bounds, bool selected = false) : selected_(selected) {
    this->bounds = bounds;
  }

  /// Wired by RadioGroup::addButton(); not meant to be set directly.
  std::function<void()> onSelected;

  bool selected() const { return selected_; }
  void setSelected(bool selected) { selected_ = selected; }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    RGB_T ring = selected_ ? this->theme().colors.primary : this->theme().colors.outline;
    RGB_T dot = this->theme().colors.primary;
    if (!this->enabled) {
      ring = blend(ring, this->theme().colors.surface, 0.5f);
      dot = blend(dot, this->theme().colors.surface, 0.4f);
    }

    const int32_t diameter = this->bounds.w < this->bounds.h ? this->bounds.w : this->bounds.h;
    target.drawCircle(toPx(this->bounds.centerX()), toPx(this->bounds.centerY()),
                      toPx(diameter / 2), ring);
    if (selected_) {
      drawDot(target, this->bounds, dot);
    }
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (!selected_) {
      selected_ = true;
      if (onSelected) onSelected();
    }
    return true;
  }

 private:
  bool selected_ = false;
};

/// Coordinates a set of RadioButtons so selecting one deselects the rest.
/// Not itself a Widget - each RadioButton must still be added to the
/// Screen individually, e.g.:
///
///   RadioGroup<RGB565> group;
///   group.onChange = [](int index) { ... };
///   for (auto& button : buttons) { screen.addWidget(button); group.addButton(button); }
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class RadioGroup {
 public:
  std::function<void(int)> onChange;

  void addButton(RadioButton<RGB_T>& button) {
    const int index = static_cast<int>(buttons_.size());
    buttons_.push_back(&button);
    button.onSelected = [this, index]() { select(index); };
  }

  int selectedIndex() const {
    for (size_t i = 0; i < buttons_.size(); ++i) {
      if (buttons_[i]->selected()) return static_cast<int>(i);
    }
    return -1;
  }

 private:
  std::vector<RadioButton<RGB_T>*> buttons_;

  void select(int index) {
    for (size_t i = 0; i < buttons_.size(); ++i) {
      buttons_[i]->setSelected(static_cast<int>(i) == index);
    }
    if (onChange) onChange(index);
  }
};

using RadioButtonRGB565 = RadioButton<tinygpu::RGB565>;
using RadioButtonRGB666 = RadioButton<tinygpu::RGB666>;
using RadioButtonRGB888 = RadioButton<tinygpu::RGB888>;
using RadioGroupRGB565 = RadioGroup<tinygpu::RGB565>;
using RadioGroupRGB666 = RadioGroup<tinygpu::RGB666>;
using RadioGroupRGB888 = RadioGroup<tinygpu::RGB888>;

}  // namespace tinymd
