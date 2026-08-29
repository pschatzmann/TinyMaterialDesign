find_package(SDL2 REQUIRED)

arduino_sketch(tab-bar tab-bar.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(tab-bar PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
