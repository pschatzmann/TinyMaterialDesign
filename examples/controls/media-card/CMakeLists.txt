find_package(SDL2 REQUIRED)

arduino_sketch(media-card media-card.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(media-card PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
