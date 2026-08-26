#pragma once

namespace tinymd {

/**
 * @brief Minimal interface a Keyboard drives.
 *
 * TextField and TextArea both implement this alongside Widget<RGB_T>, so a
 * single Keyboard can serve either kind of field interchangeably via
 * Keyboard::manage() - it only ever talks to its target through this
 * interface, never caring which concrete widget it's driving.
 *
 * Not templated on RGB_T: character insertion has nothing to do with pixel
 * format, and keeping this interface non-generic is what lets Keyboard
 * (which *is* templated on RGB_T, since it draws itself) hold a single
 * `TextInputTarget*` regardless of the field's color type.
 */
class TextInputTarget {
 public:
  virtual ~TextInputTarget() = default;

  virtual void insertChar(char c) = 0;
  virtual void backspace() = 0;
  virtual void blur() = 0;

  /// Called when the keyboard's Done key is tapped (a separate Enter key
  /// handles literal newlines via insertChar('\n') directly, without going
  /// through this). @return true if the keyboard should hide itself
  /// afterward - both TextField and TextArea return true here, since Done
  /// always means "finished editing", but the interface leaves room for a
  /// future field that wants Done to keep editing open instead.
  virtual bool handleEnter() = 0;
};

}  // namespace tinymd
