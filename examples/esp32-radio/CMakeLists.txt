# Desktop debug build - see esp32-radio.ino's header comment. UI layout and
# rendering (including the genre/country JPEG thumbnails) work the same as
# on the real board; station search and audio playback also run for real
# here, via arduino-audio-tools on top of the Arduino-Emulator's WiFi/socket
# stubs (real host networking) and PortAudio for actual sound output - only
# SD-card recording (not implemented at all, see the .ino) and the real
# ILI9341/ES8311 hardware itself are unavailable on a desktop.

find_package(SDL2 REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Fetch TinyJPEG from GitHub, matching TinyGPU's own
# examples/jpeg-parser-demo/CMakeLists.txt FetchContent pattern. TinyJPEG is
# header-only (INTERFACE target), so this just adds it to the include path -
# no sources to compile.
if(NOT TARGET TinyJPEG)
  FetchContent_Declare(
    TinyJPEG
    GIT_REPOSITORY https://github.com/pschatzmann/TinyJPEG.git
    GIT_TAG        main
  )
  FetchContent_MakeAvailable(TinyJPEG)
endif()

# arduino-audio-tools itself is header-only, but its CMakeLists.txt also
# offers to fetch its own dependencies - PortAudio (real desktop sound
# output, see PortAudioStream in the .ino) and, redundantly, its own
# Arduino-Emulator copy (ADD_ARDUINO_EMULATOR is ON by default) - we
# already have arduino_emulator from TinyMaterialDesign's own top-level
# CMakeLists.txt (built with USE_HTTPS, see there), so that's turned off
# here to avoid fetching/building it twice.
if(NOT TARGET arduino-audio-tools)
  set(ADD_PORTAUDIO ON CACHE BOOL "" FORCE)
  set(ADD_ARDUINO_EMULATOR OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    arduino_audio_tools
    GIT_REPOSITORY https://github.com/pschatzmann/arduino-audio-tools.git
    GIT_TAG        main
  )
  FetchContent_MakeAvailable(arduino_audio_tools)
endif()

# MP3DecoderHelix's underlying decoder - matches arduino-audio-tools' own
# examples/examples-desktop/mp3/CMakeLists.txt FetchContent pattern.
if(NOT TARGET arduino_helix)
  FetchContent_Declare(
    arduino_helix
    GIT_REPOSITORY https://github.com/pschatzmann/arduino-libhelix.git
    GIT_TAG        main
  )
  FetchContent_MakeAvailable(arduino_helix)
  target_compile_definitions(arduino_helix PUBLIC -DARDUINO -DHELIX_LOGGING_ACTIVE=0)
endif()

# wolfssl (the CMake target, not just the static lib bytes already pulled
# in transitively via arduino_emulator) also needs to be linked here
# directly: Arduino-Emulator's own CMakeLists.txt turns USE_HTTPS on via
# add_definitions(-DUSE_HTTPS ...), a directory-scoped call that only
# affects arduino_emulator's own .cpp files, not ours - so without
# defining USE_HTTPS ourselves below too, our own #include <WiFi.h> would
# still see WiFiClientSecure.h's #else (no-TLS) branch. Linking wolfssl
# brings in its PUBLIC include dirs (its checkout plus its own build dir,
# where wolfssl/options.h gets generated), needed once USE_HTTPS makes
# our translation unit start including <wolfssl/ssl.h> too.
arduino_sketch(esp32-radio esp32-radio.ino
               LIBRARIES TinyMaterialDesign SDL2::SDL2 TinyJPEG
                         arduino-audio-tools arduino_helix portaudio wolfssl)
# IS_DESKTOP selects arduino-audio-tools' own desktop platform config
# (AudioTools/PlatformConfig/desktop.h) - without it, AudioTools assumes a
# real microcontroller (HardwareSerial, digitalPinToInterrupt(), ...) that
# the Arduino-Emulator doesn't provide. TMD_REAL_WIFICLIENTSECURE enables
# arduino-audio-tools' own desktop-platform support (upstream as of the
# "Desktop corrections" commit) for a real TLS WiFiClientSecure instead of
# a fake one, and for skipping certificate validation to match how this
# project already treats all station TLS as demo-grade.
target_compile_definitions(esp32-radio PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE IS_DESKTOP
                           TMD_REAL_WIFICLIENTSECURE USE_HTTPS)
# wolfssl/wolfcrypt/settings.h forces WOLFSSL_USER_SETTINGS mode (expecting
# a user_settings.h) whenever ARDUINO is defined, which this sketch does -
# see wolfssl-user-settings-shim/user_settings.h for why this redirects
# that to Arduino-Emulator's own generated wolfssl/options.h instead.
target_include_directories(esp32-radio PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}/wolfssl-user-settings-shim)
