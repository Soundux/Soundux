cmake_minimum_required(VERSION 3.1)
project(tray VERSION 0.2 DESCRIPTION "A cross-platform C++ system tray library")

file(GLOB src
    "example/simple/main.cpp"
    "tray/src/*.cpp"
    "tray/src/*/*.cpp"
    "tray/src/*/*/*.cpp"
)

add_library(tray STATIC ${src})

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(APPINDICATOR REQUIRED ayatana-appindicator3-0.1)

    target_link_libraries(tray INTERFACE ${GTK3_LIBRARIES} ${APPINDICATOR_LIBRARIES})

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(tray PRIVATE -Wall -Wextra -Werror -pedantic -Wno-unused-lambda-capture)
    endif()
    
    target_include_directories(tray SYSTEM PUBLIC ${GTK3_INCLUDE_DIRS} ${APPINDICATOR_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})
endif()

target_include_directories(tray SYSTEM PUBLIC "tray/include")

target_compile_features(tray PRIVATE cxx_std_17)
set_target_properties(tray PROPERTIES 
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF
                      CXX_STANDARD_REQUIRED ON)

set_target_properties(tray PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(tray PROPERTIES PROJECT_NAME ${PROJECT_NAME})
