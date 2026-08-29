find_package(SDL2 REQUIRED)

arduino_sketch(text-area text-area.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(text-area PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
