cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 1.0 DESCRIPTION "")

file(GLOB src
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    "src/*/*/*/*.cpp"
)

add_executable(soundux ${src})

target_include_directories(soundux PRIVATE "lib/miniaudio")
target_include_directories(soundux PRIVATE "lib/fancypp/include")
target_include_directories(soundux PRIVATE "lib/json/single_include")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(soundux PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if (UNIX)
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(soundux PRIVATE ${X11_LIBRARIES} ${X11_Xinput_LIB})
endif()

add_subdirectory(src/ui/impl/webview/lib/webview)
add_subdirectory(lib/nativefiledialog-extended)
target_link_libraries(soundux PUBLIC webview nfd)

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})
