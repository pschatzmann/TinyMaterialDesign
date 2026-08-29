find_package(SDL2 REQUIRED)

arduino_sketch(fab fab.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(fab PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
