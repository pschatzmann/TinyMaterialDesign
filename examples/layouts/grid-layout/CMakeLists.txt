find_package(SDL2 REQUIRED)

arduino_sketch(grid-layout grid-layout.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(grid-layout PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
