find_package(SDL2 REQUIRED)

arduino_sketch(line-chart line-chart.ino LIBRARIES TinyMaterialDesign SDL2::SDL2)
target_compile_definitions(line-chart PRIVATE TINYGPU_AUTO_NAMESPACE
                           TINYMD_AUTO_NAMESPACE)
