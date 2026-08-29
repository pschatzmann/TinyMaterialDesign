find_package(SDL2 REQUIRED)

arduino_sketch(circular-progress-indicator circular-progress-indicator.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(circular-progress-indicator PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
