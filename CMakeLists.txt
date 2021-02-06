cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 0.1.6 DESCRIPTION "A cross-platform soundboard in QtQuick")

# Options
option(SOUNDUX_DEBUGINFO "Compiles with debug symbols to provide useful information for debugging" Off)
if (SOUNDUX_DEBUGINFO)
    message("Compiling with debug info")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
# -------

# Generate Material Design Js
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/src/qml/resources/MaterialDesign.js")
    if (MSVC)
        execute_process(COMMAND cmd /c python "${CMAKE_SOURCE_DIR}/lib/generateMaterialDesignJs.py"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    else()
        execute_process(COMMAND python3 "${CMAKE_SOURCE_DIR}/lib/generateMaterialDesignJs.py"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    endif()

    message("Generated MaterialDesign.js")
endif()
# ---------------------------

# QT-Related
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Widgets Qml Quick QuickControls2 REQUIRED)
qt5_add_resources(QT_RESOURCES src/qml/qml.qrc)
# ----------

# Source Files to compile
file(GLOB src
    "src/*.cpp"
    "src/*.qml"
    "src/*/*.cpp"
)
# -----------------------

# Linux Dependencies
if (UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
endif()
# ------------------

add_executable(soundux ${src} ${QT_RESOURCES})

target_include_directories(soundux PRIVATE ${Qt5Widgets_INCLUDE_DIRS} ${QtQml_INCLUDE_DIRS})
if (UNIX)
    target_include_directories(soundux PRIVATE ${X11_INCLUDE_DIR} ${X11_Xinput_INCLUDE_PATH})
endif()
target_include_directories(soundux PRIVATE "lib/json/single_include/nlohmann")
target_include_directories(soundux PRIVATE "lib/object_threadsafe")
target_include_directories(soundux PRIVATE "lib/miniaudio")

target_compile_definitions(soundux PRIVATE ${Qt5Widgets_DEFINITIONS} ${QtQml_DEFINITIONS} ${Qt5Quick_DEFINITIONS})

if (UNIX)
    target_link_libraries(soundux ${X11_LIBRARIES} ${X11_Xinput_LIB} ${CMAKE_DL_LIBS})
    target_link_libraries(soundux pthread)
endif()

target_link_libraries(soundux
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
)

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})

install(TARGETS soundux DESTINATION bin)