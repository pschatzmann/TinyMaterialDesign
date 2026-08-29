find_package(SDL2 REQUIRED)

arduino_sketch(carousel carousel.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(carousel PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
