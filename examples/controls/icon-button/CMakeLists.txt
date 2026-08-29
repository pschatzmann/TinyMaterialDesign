find_package(SDL2 REQUIRED)

arduino_sketch(icon-button icon-button.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(icon-button PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
