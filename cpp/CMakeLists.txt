cmake_minimum_required(VERSION 3.20)
project(RiverOfLamps VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    message(FATAL_ERROR
        "The vendored SFML static libraries are built for MinGW-w64, not MSVC. "
        "Use the MinGW-w64 toolchain (see build_cpp.bat), or install SFML 2.6.1 "
        "via vcpkg and adapt cpp/CMakeLists.txt for the MSVC import targets.")
endif()

set(SFML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SFML-2.6.1")
set(SFML_LIBS
    "${SFML_ROOT}/lib/libsfml-graphics-s.a"
    "${SFML_ROOT}/lib/libsfml-window-s.a"
    "${SFML_ROOT}/lib/libsfml-system-s.a"
    "${SFML_ROOT}/lib/libfreetype.a"
    "${SFML_ROOT}/lib/libsfml-main.a"
    opengl32
    gdi32
    winmm
)

add_library(river_core STATIC
    src/core/component_rules.cpp
    src/core/circuit_graph.cpp
    src/core/level_builder.cpp
    src/core/level_repository.cpp
    src/core/simulator.cpp
    src/generated/embedded_json.cpp
)
target_include_directories(river_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)
target_compile_features(river_core PUBLIC cxx_std_20)

add_executable(RiverOfLamps WIN32
    src/main.cpp
    src/app/app.cpp
    src/app/board_view.cpp
    src/app/progress.cpp
    src/app/sound.cpp
    src/app/ui_draw.cpp
    src/app/waveform_view.cpp
    src/generated/embedded_font.cpp
)
target_include_directories(RiverOfLamps PRIVATE "${SFML_ROOT}/include")
target_compile_definitions(RiverOfLamps PRIVATE SFML_STATIC)
target_link_libraries(RiverOfLamps PRIVATE river_core ${SFML_LIBS})

add_executable(river_tests
    tests/main.cpp
    src/app/progress.cpp
)
target_link_libraries(river_tests PRIVATE river_core)

if(MINGW)
    target_compile_options(RiverOfLamps PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    target_compile_options(river_tests PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    target_link_options(RiverOfLamps PRIVATE -static -static-libgcc -static-libstdc++ -mwindows)
    target_link_options(river_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set_target_properties(RiverOfLamps PROPERTIES OUTPUT_NAME "RiverOfLamps")
set_target_properties(river_tests PROPERTIES OUTPUT_NAME "river_tests")

enable_testing()
add_test(NAME river_tests COMMAND river_tests)

message(STATUS "SFML root: ${SFML_ROOT}")
message(STATUS "Build output: ${CMAKE_BINARY_DIR}")
