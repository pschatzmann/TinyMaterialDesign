cmake_minimum_required(VERSION 3.14)

if (DEFINED ESP_PLATFORM)

    # ESP-IDF component build. Requires a "TinyGPU" component in the same
    # build (e.g. cloned as a sibling under components/, or added via
    # EXTRA_COMPONENT_DIRS) - see idf_component.yml.
    set(TMD_REQUIRES TinyGPU)
    if(TARGET arduino-esp32::arduino-esp32)
        list(APPEND TMD_REQUIRES arduino-esp32)
    endif()

    idf_component_register(
        INCLUDE_DIRS src
        REQUIRES ${TMD_REQUIRES}
    )

else()

    project(TinyMaterialDesign LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    option(TMD_BUILD_EXAMPLES "Build Examples" ON)

    # TinyGPU builds its own examples (font-test, bouncing-ball, ...) when
    # TGPU_BUILD_EXAMPLES is ON, which is its default. We only want our own
    # examples/ built, so force it off before fetching TinyGPU.
    set(TGPU_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    # Fetch TinyGPU from GitHub. Override with
    # -DFETCHCONTENT_SOURCE_DIR_TINYGPU=/path/to/local/TinyGPU to build against a
    # local checkout instead (e.g. during development of both libraries side by
    # side), which also skips the network fetch below.
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
      tinygpu
      GIT_REPOSITORY https://github.com/pschatzmann/TinyGPU.git
      GIT_TAG        main
    )
    FetchContent_MakeAvailable(tinygpu)

    # Create TinyMaterialDesign as a header-only interface library
    add_library(TinyMaterialDesign INTERFACE)
    target_link_libraries(TinyMaterialDesign INTERFACE TinyGPU)
    target_include_directories(TinyMaterialDesign INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    if(TMD_BUILD_EXAMPLES)
      # Our examples are Arduino sketches built via arduino_sketch(), which
      # TinyGPU only pulls in (from Arduino-Emulator) when it builds its own
      # examples - since we just disabled that, fetch it ourselves.
      FetchContent_Declare(
        arduino_emulator
        GIT_REPOSITORY https://github.com/pschatzmann/Arduino-Emulator.git
        GIT_TAG        main
      )
      FetchContent_MakeAvailable(arduino_emulator)

      add_subdirectory(examples)
    endif()

endif()
