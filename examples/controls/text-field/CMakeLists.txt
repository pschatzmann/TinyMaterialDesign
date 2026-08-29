find_package(SDL2 REQUIRED)

arduino_sketch(text-field text-field.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(text-field PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
