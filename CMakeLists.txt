cmake_minimum_required(VERSION 3.5)

project(Soundboard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Threads)

SET(CMAKE_CXX_FLAGS "-O0")
SET(CMAKE_C_FLAGS "-O0")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(Soundboard
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  json.hpp
  resources.qrc
)

target_link_libraries(Soundboard PRIVATE Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT})
