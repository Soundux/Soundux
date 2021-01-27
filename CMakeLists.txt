cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 1.0 DESCRIPTION "")

file(GLOB src
    "src/*.cpp"
    "src/*/*.cpp"
)

add_executable(soundux ${src})
target_include_directories(soundux PRIVATE "lib/json/single_include")

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})