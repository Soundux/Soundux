cmake_minimum_required(VERSION 3.0.0)
project(soundux VERSION 0.1.3)

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/src/MaterialDesign.js")
    if (MSVC)
    execute_process(COMMAND cmd /c python "${CMAKE_SOURCE_DIR}/lib/generateMaterialDesignJs.py"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    else()
    execute_process(COMMAND python3 "${CMAKE_SOURCE_DIR}/lib/generateMaterialDesignJs.py"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    endif()
    message("Generated MaterialDesign.js")
endif()

file(GLOB src
    "src/*.cpp"
    "src/*.qml"
    "src/*/*.cpp"
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Widgets Qml Quick QuickControls2 REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS} ${QtQml_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS} ${QtQml_DEFINITIONS} ${${Qt5Quick_DEFINITIONS}})
qt5_add_resources(QT_RESOURCES src/qml.qrc)

if (UNIX)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
include_directories(${X11_INCLUDE_DIR} ${X11_Xinput_INCLUDE_PATH})
endif(UNIX)

include_directories("lib/json/single_include/nlohmann")
include_directories("lib/object_threadsafe")
include_directories("lib/miniaudio")

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
endif(MSVC)

add_executable(soundux ${src} ${QT_RESOURCES})

install(TARGETS soundux DESTINATION bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

if (UNIX)
target_link_libraries(soundux ${X11_LIBRARIES} ${X11_Xinput_LIB} ${CMAKE_DL_LIBS})
target_link_libraries(soundux pthread)
endif(UNIX)

target_link_libraries(soundux
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
)

set(CMAKE_CXX_STANDARD 17)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
