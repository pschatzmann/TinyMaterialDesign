find_package(SDL2 REQUIRED)

arduino_sketch(virtualized-list virtualized-list.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(virtualized-list PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
