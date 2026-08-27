find_package(SDL2 REQUIRED)

arduino_sketch(kitchen-sink kitchen-sink.ino
               LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(kitchen-sink PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
