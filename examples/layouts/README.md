# layouts

One short, self-contained sketch per TinyMaterialDesign layout calculator
(`GridLayout`, `LinearLayout`, `FlowLayout`, `SplitLayout`, `AnchorLayout`,
`StackLayout`, `RadialLayout`, `TableLayout` - see `docs/Tutorial.md`'s
Layouts chapter). None of these are widgets: each sketch just uses the
layout to compute a few widgets' `Bounds` in `setup()`, then registers those
widgets with `Screen` normally - see `Core/GridLayout.h` for why layouts
don't own or draw anything themselves. `Container` (a real, nestable,
auto-scrolling widget) lives under `examples/controls/container` instead,
since it isn't a rect calculator.

Every sketch builds and runs unchanged on a real ESP32 board and, via
`LCDBoardDesktopSDL`, in a desktop SDL2 window - see
`../kitchen-sink/kitchen-sink.ino` for the combined demo and the
board-selection pattern each of these follows.

Build and run one, e.g.:

```sh
cmake -B build -S ../..
cmake --build build --target grid-layout
./build/examples/layouts/grid-layout/grid-layout
```
