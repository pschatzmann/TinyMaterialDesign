#pragma once
// wolfSSL's wolfcrypt/settings.h forces WOLFSSL_USER_SETTINGS mode (and so
// `#include "user_settings.h"`) whenever ARDUINO is defined - true for this
// sketch, even on the desktop debug build (see examples/esp32-radio's
// CMakeLists.txt and esp32-radio.ino's own comments on IS_DESKTOP). But
// Arduino-Emulator's own wolfssl CMake fetch generates wolfssl/options.h,
// not user_settings.h - this shim (only reachable via this directory being
// on the include path, see CMakeLists.txt) just forwards to that.
#include <wolfssl/options.h>
