find_package(SDL2 REQUIRED)

arduino_sketch(switch switch.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(switch PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
