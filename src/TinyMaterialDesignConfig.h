#pragma once
// Every examples/controls and examples/layouts sketch logs taps/clicks via
// a plain printf() in its widget callbacks, without including <cstdio>
// itself - guaranteed here rather than relying on it happening to already
// be pulled in transitively by some TinyGPU internal.
#include <cstdio>
#include "TinyGPU/Boards/LCDBoards.h"

namespace tinymd {

using TINYMD_DEFAULT_RGB_T = tinygpu::RGB565;

#ifdef ESP32
/// The board every examples/controls and examples/layouts sketch uses by
/// default - an actual object (not just a type), already fully
/// constructed, so a sketch never needs its own `#ifdef ESP32` to build
/// one: `App<RGB565> app(DefaultBoard);` is all it takes, on every
/// platform. Swap in a different `LCDBoard` from
/// `TinyGPU/Boards/LCDBoardsESP32.h` for other real hardware by
/// constructing and using your own instead (see kitchen-sink.ino/
/// esp32-radio.ino, which do this because they need more than one board
/// type or extra wiring `App` doesn't cover).
inline tinygpu::LCDBoardGuitionESP32_LVGL_2_4Display DefaultBoard;
#elif __has_include(<SDL.h>)
/// See the `#ifdef ESP32` branch above - on desktop this is an
/// `LCDBoardDesktopSDL` sized 240x320. Deliberately not named `kWidth`/
/// `kHeight` here (even as an internal-looking constant) - a sketch with
/// its own same-named globals, like kitchen-sink.ino, would collide with
/// them through `using namespace tinymd`; a sketch reads the board's real
/// size back via `App::width()`/`height()` instead.
inline tinygpu::LCDBoardDesktopSDL DefaultBoard(240, 320);
#endif

}  // namespace tinymd
