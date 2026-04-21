cmake_minimum_required(VERSION 3.1)
project(tray-example VERSION 0.1)

add_executable(tray-example "main.cpp")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../ ${CMAKE_CURRENT_SOURCE_DIR}/../../example_build EXCLUDE_FROM_ALL)
target_link_libraries(tray-example tray)

target_compile_features(tray-example PUBLIC cxx_std_17)
set_target_properties(tray-example PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(tray-example PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(tray-example PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)
set_target_properties(tray-example PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(tray-example PROPERTIES PROJECT_NAME ${PROJECT_NAME})