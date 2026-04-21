cmake_minimum_required(VERSION 3.1)
project(webview-example VERSION 0.1)

add_executable(webview-example "main.cpp")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../ ${CMAKE_CURRENT_SOURCE_DIR}/../../example_build EXCLUDE_FROM_ALL)

target_link_libraries(webview-example webview)
target_include_directories(webview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # This directory contains the embedded folder - thats why we have to add it as a include directory 

target_compile_features(webview-example PUBLIC cxx_std_17)
set_target_properties(webview-example PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(webview-example PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(webview-example PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)
set_target_properties(webview-example PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(webview-example PROPERTIES PROJECT_NAME ${PROJECT_NAME})