cmake_minimum_required(VERSION 3.0.0)
project(embed_helper VERSION 0.1.0)

add_executable(embed_helper main.cpp)

target_compile_features(embed_helper PRIVATE cxx_std_17)
set_target_properties(embed_helper PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(embed_helper PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(embed_helper PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(embed_helper PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(embed_helper PROPERTIES PROJECT_NAME ${PROJECT_NAME})