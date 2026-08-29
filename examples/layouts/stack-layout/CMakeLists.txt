find_package(SDL2 REQUIRED)

arduino_sketch(stack-layout stack-layout.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(stack-layout PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
