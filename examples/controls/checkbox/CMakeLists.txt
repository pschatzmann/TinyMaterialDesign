find_package(SDL2 REQUIRED)

arduino_sketch(checkbox checkbox.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(checkbox PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
