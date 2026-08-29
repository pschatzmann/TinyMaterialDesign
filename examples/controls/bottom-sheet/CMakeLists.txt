find_package(SDL2 REQUIRED)

arduino_sketch(bottom-sheet bottom-sheet.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(bottom-sheet PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
