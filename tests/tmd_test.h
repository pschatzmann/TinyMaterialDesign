#pragma once
#include <cstdio>

// Minimal, dependency-free test helper. Each test .cpp defines its own
// main(), sets `tmd_test_ok = true` at the top, uses TMD_CHECK() for every
// assertion (all of them run - unlike assert(), one failure doesn't stop
// the rest of the checks in the same test from also reporting), and
// returns tmd_test_ok ? 0 : 1 at the end - the pass/fail signal CTest
// reads (see tests/CMakeLists.txt's add_test()).
static bool tmd_test_ok = true;

#define TMD_CHECK(cond)                                                                    \
  do {                                                                                     \
    if (!(cond)) {                                                                         \
      fprintf(stderr, "CHECK FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__);            \
      tmd_test_ok = false;                                                                 \
    }                                                                                       \
  } while (0)
