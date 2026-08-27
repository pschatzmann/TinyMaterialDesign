find_package(SDL2 REQUIRED)

arduino_sketch(desktop-kitchen-sink desktop-kitchen-sink.ino
               LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(desktop-kitchen-sink PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
