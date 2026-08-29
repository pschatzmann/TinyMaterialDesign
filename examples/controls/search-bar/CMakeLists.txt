find_package(SDL2 REQUIRED)

arduino_sketch(search-bar search-bar.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(search-bar PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
