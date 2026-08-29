find_package(SDL2 REQUIRED)

arduino_sketch(navigation-rail navigation-rail.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(navigation-rail PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
