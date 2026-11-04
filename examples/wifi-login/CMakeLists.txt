find_package(SDL2 REQUIRED)

arduino_sketch(wifi-login wifi-login.ino
               LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(wifi-login PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
