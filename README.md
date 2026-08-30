# TinyMaterialDesign

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![Build with CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake)](https://cmake.org/)
[![ESP-IDF Component](https://img.shields.io/badge/ESP--IDF-Component-blue.svg?logo=espressif)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html#components)
[![License: Apache](https://img.shields.io/badge/License-Apache-yellow.svg)](https://opensource.org/licenses/Apache-2.0)

Material Design is an open-source design system created by Google in 2014 to help developers and designers build consistent, high-quality digital interfaces across Android, iOS, the web, and other platforms.

TinyMaterialDesign is a header-only Arduino library that implements the most
important [Material Design 3](https://m3.material.io/) GUI components and
draws them with [TinyGPU](https://github.com/pschatzmann/TinyGPU) - using
TinyGPU's surfaces for drawing and its `TouchDriver`/`GestureDetector` stack
for touch interaction.

![alt text](image.png)

## Features

- Material 3 theming - color schemes (including light/dark variants and
  seed-hue presets), shape/spacing tokens, and typography, all backed by
  TinyGPU's bitmap fonts
- Works with RGB565, RGB666, and RGB888 surfaces
- Over 30 Material 3 widgets covering actions, selection, input, progress,
  containment, navigation, and text entry - including a nestable, scrolling
  `Container` and `Screen` (which is itself one)
- A small set of layout calculators (`Core/*Layout.h`) for positioning
  widgets - grid, linear, flow, split, anchor, stack, radial, and table
- Automatic scrolling with clipping, correct gesture routing (drag latching,
  nested-widget dispatch) at any nesting depth, and an optional
  callback-driven "virtualized content" mode for lists too large to keep
  every item resident in memory

## Documentation
- [Tutorial](https://github.com/pschatzmann/TinyMaterialDesign/wiki/TinyMaterialDesign-Tutorial)
- [Examples](https://github.com/pschatzmann/TinyMaterialDesign/tree/main/examples)
- [Wiki](https://github.com/pschatzmann/TinyMaterialDesign/wiki)
- [Class Documentaion](https://pschatzmann.github.io/TinyMaterialDesign/annotated.html)


## Requirements

- [TinyGPU](https://github.com/pschatzmann/TinyGPU) 0.4.0 or newer (this
  library uses TinyGPU's `fillRoundRect`/`drawRoundRect`/`drawArc`/
  `pushClipRect`/`popClipRect`)

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

