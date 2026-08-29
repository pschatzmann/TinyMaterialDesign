# TinyMaterialDesign

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![Build with CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake)](https://cmake.org/)
[![ESP-IDF Component](https://img.shields.io/badge/ESP--IDF-Component-blue.svg?logo=espressif)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html#components)
[![License: Apache](https://img.shields.io/badge/License-Apache-yellow.svg)](https://opensource.org/licenses/Apache-2.0)

TinyMaterialDesign is a header-only Arduino library that implements the most
important [Material Design 3](https://m3.material.io/) GUI components and
draws them with [TinyGPU](https://github.com/pschatzmann/TinyGPU) - using
TinyGPU's surfaces for drawing and its `TouchDriver`/`GestureDetector` stack
for touch interaction.

![alt text](image.png)

## Features

- Material 3 theming: `ColorScheme` (the baseline purple palette, plus
  seed-hue-generated Blue/Green/Red/Orange presets, each with a light and
  dark variant - see `MaterialTheme.h`'s `defaultTheme()`/`blueTheme()`/
  `greenTheme()`/`redTheme()`/`orangeTheme()` and their `*DarkTheme()`
  counterparts), shape (corner radius) tokens, an 8px spacing unit, and 4
  typography roles backed by TinyGPU's bitmap fonts
- Works with RGB565, RGB666 and RGB888 surfaces (every class is a template
  over `RGB_T`, same convention TinyGPU itself uses)
- Widgets:
  - `Button` (Filled / Tonal / Outlined / Text / Elevated, with a ripple)
  - `IconButton` (Standard / Filled / Tonal - Filled/Tonal are an elevated,
    colored circular button), `FloatingActionButton`/`FAB` (circular or
    extended pill with a label, in Surface/Primary/Secondary/Tertiary
    colors), `Checkbox`, `Switch`, `RadioButton` + `RadioGroup`
  - `Slider` (drag or tap-to-jump), `SegmentedButton` (single- or
    multi-select connected segments)
  - `LinearProgressIndicator` / `CircularProgressIndicator` (determinate or
    indeterminate)
  - `Card` (elevated, with word-wrapped body text), `Label`, `Divider`,
    `Chip`, `Badge` (dot or count marker)
  - `AppBar` (title + leading/trailing icon), `TabBar` (exclusive tab row
    with a sliding indicator)
  - `Dialog` (modal alert), `Menu` (popover item list, modal but unscrimmed),
    `BottomSheet` (modal bottom panel with a drag handle)
  - `Drawer` (modal side navigation panel, shown like a `Dialog`) + `ListItem`
    (tappable icon+title row with a selected state - the drawer's building
    block, also usable standalone for any settings-style list)
  - `NavigationBar` / `NavigationRail` (bottom/side exclusive destination
    navigation, icon + label with a selected pill)
  - `Snackbar` (auto-dismissing bottom message + action), `Tooltip` (timed
    popup label anchored to a widget), `Banner` (persistent inline message
    with up to 2 actions)
  - `MediaCard` (tappable, image-backed card with a caption - pair with
    `GridLayout`, Core/GridLayout.h, to lay out several of these in a
    wrapping grid)
  - `TextField` (single-line), `TextArea` (multi-line, word-wrapped) and
    `SearchBar` (pill-shaped, with search/clear glyphs) text input, plus
    `Keyboard` (on-screen QWERTY, with a symbols page and a dedicated
    Enter/newline key) - one `Keyboard` can drive any number of fields via
    `keyboard.manage(field)`
- `Screen`: owns your widgets, draws them, and routes gesture events to the
  right one - including correctly latching a `Slider` drag even if the
  finger moves past the slider's own bounds. Content added via
  `addWidget()` scrolls automatically (drag up/down) when it's taller than
  the drawn surface, with an auto-appearing scrollbar; `addFixedWidget()`
  pins a widget (an `AppBar`, a `Keyboard`) to its own screen position
  regardless of scroll

## Documentation

See [`docs/Tutorial.md`](docs/Tutorial.md) for an introduction to the
library's core concepts (`Widget`, `MaterialTheme`, `Screen`) and a
guided, per-control reference - what each widget is for, usage guidelines,
example code, and a screenshot for every one of them.

See `examples/kitchen-sink` for a fully worked, clickable demo (one of every
widget, with scrolling and a color scheme picker) that runs unchanged on
both a real ESP32 board (`LCDBoardGuitionESP32_LVGL_2_4Display` by default -
swap in a different `LCDBoard` from `TinyGPU/Boards/LCDBoardsESP32.h` for
other hardware) and, via `LCDBoardDesktopSDL`, in an identically-sized SDL2
window on desktop for mouse-driven testing without touch hardware. See
`examples/controls` for a short, single-widget sketch per control, and
`examples/esp32-touch-buttons` for a smaller real-hardware-flavored sketch.


## Requirements

- [TinyGPU](https://github.com/pschatzmann/TinyGPU) 0.4.0 or newer (this
  library uses TinyGPU's `fillRoundRect`/`drawRoundRect`/`drawArc`)

## Installation

For Arduino, you can download the library as zip and use Sketch -> Include
Library -> Add .ZIP Library. Or you can git clone this project - and
[TinyGPU](https://github.com/pschatzmann/TinyGPU), which it depends on -
into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/TinyGPU.git
git clone https://github.com/pschatzmann/TinyMaterialDesign.git
```

For CMake-based projects (desktop, PlatformIO, ...), `CMakeLists.txt`
provides a `TinyMaterialDesign` INTERFACE target that fetches TinyGPU via
`FetchContent` - see [Building the examples](#building-the-examples) below.

For ESP-IDF, TinyMaterialDesign is also usable as a component: clone both
this library and TinyGPU as siblings under `components/` (or add their
parent directory via `EXTRA_COMPONENT_DIRS`) and
`idf_component_register`/`idf_component.yml` take care of the rest. Like
TinyGPU itself, this only compiles when `arduino-esp32` is also present as
a component in the same build (uncomment the dependency in
`idf_component.yml` if you need it) - TinyMaterialDesign pulls in TinyGPU's
`TouchDriver.h`/`DeviceOutput.h`, which need Arduino APIs.

## Building the examples

You can build the examples to run on the desktop using cmake:

```sh
cmake -B build -S .
cmake --build build
./build/examples/kitchen-sink/kitchen-sink
```

Pass `-DFETCHCONTENT_SOURCE_DIR_TINYGPU=/path/to/local/TinyGPU` to build
against a local TinyGPU checkout instead of fetching it from GitHub.
