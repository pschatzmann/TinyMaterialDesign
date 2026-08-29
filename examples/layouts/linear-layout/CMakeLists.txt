find_package(SDL2 REQUIRED)

arduino_sketch(linear-layout linear-layout.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(linear-layout PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
