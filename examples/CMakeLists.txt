# examples/esp32-touch-buttons is a reference-only sketch for real touch
# hardware (XPT2046/CST816S) and is intentionally not built here - there's
# no hardware or emulator path to run it against.
add_subdirectory(kitchen-sink)

# One short, self-contained sketch per widget - see examples/controls/README.md.
add_subdirectory(controls)

# One short, self-contained sketch per layout calculator - see
# examples/layouts/README.md.
add_subdirectory(layouts)

# esp32-radio.ino's #ifdef ESP32 picks LCDBoardDesktopSDL + mock station
# data here (no WiFi/HTTPS/SD/real audio in this build) instead of real
# hardware - see its own header comment.
add_subdirectory(esp32-radio)
