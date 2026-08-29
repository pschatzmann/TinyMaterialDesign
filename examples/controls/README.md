# controls

One short, self-contained sketch per TinyMaterialDesign widget - each shows
exactly one control (plus, where the widget requires it to function, the
minimal collaborator its own API expects: a `Button` action for `Dialog`, a
`ListItem` for `Drawer`/`Menu`/`BottomSheet`, ...). Every sketch builds and
runs unchanged on a real ESP32 board and, via `LCDBoardDesktopSDL`, in a
desktop SDL2 window - see `../kitchen-sink/kitchen-sink.ino` for the combined
demo and the board-selection pattern each of these follows.

Build and run one, e.g.:

```sh
cmake -B build -S ../..
cmake --build build --target button
./build/examples/controls/button/button
```
