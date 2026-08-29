find_package(SDL2 REQUIRED)

arduino_sketch(slider slider.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(slider PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
