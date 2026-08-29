find_package(SDL2 REQUIRED)

arduino_sketch(linear-progress-indicator linear-progress-indicator.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(linear-progress-indicator PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
