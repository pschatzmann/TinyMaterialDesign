# Plain logic/behavior tests against the TinyMaterialDesign headers - no
# board, display, or Arduino emulation needed (everything here draws into a
# real in-memory TinyGPU Surface, exactly like the desktop examples do, just
# without SDL or a window). Each test_*.cpp is its own executable and its
# own CTest case; add a new one just by dropping a file in this directory.
file(GLOB TMD_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

foreach(test_source ${TMD_TEST_SOURCES})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  # arduino_emulator: see the top-level CMakeLists.txt comment - TinyGPU.h
  # needs its ArduinoCore-API include path even for a board-free test.
  target_link_libraries(${test_name} PRIVATE TinyMaterialDesign arduino_emulator)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
