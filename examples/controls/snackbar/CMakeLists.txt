find_package(SDL2 REQUIRED)

arduino_sketch(snackbar snackbar.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(snackbar PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
