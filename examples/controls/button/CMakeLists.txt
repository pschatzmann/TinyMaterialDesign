find_package(SDL2 REQUIRED)

arduino_sketch(button button.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(button PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
