find_package(SDL2 REQUIRED)

arduino_sketch(list-item list-item.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(list-item PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
