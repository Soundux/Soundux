cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 1.0 DESCRIPTION "")

file(GLOB src
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    "src/*/*/*/*.cpp"
)

if (WIN32)
add_executable(soundux WIN32 ${src})
else()
add_executable(soundux ${src})
endif()

target_include_directories(soundux PRIVATE "lib/miniaudio")
target_include_directories(soundux PRIVATE "lib/fancypp/include")
target_include_directories(soundux PRIVATE "lib/json/single_include")
target_include_directories(soundux PRIVATE "lib/InstanceGuard/Source")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(soundux PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if (UNIX)
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(soundux PRIVATE ${X11_LIBRARIES} ${X11_Xinput_LIB})
endif()
if (WIN32)
target_compile_definitions(soundux PRIVATE _CRT_SECURE_NO_WARNINGS=1 _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _UNICODE=1)
endif()

add_subdirectory(src/ui/impl/webview/lib/webview)
add_subdirectory(lib/nativefiledialog-extended)
add_subdirectory(lib/InstanceGuard/Source)
target_link_libraries(soundux PUBLIC webview nfd InstanceGuard)

# [[ Build Frontend ]]
if (MSVC)
execute_process(COMMAND cmd /c yarn
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/"
                OUTPUT_QUIET)
execute_process(COMMAND cmd /c yarn build
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/"
                OUTPUT_QUIET)
file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/dist" 
    DESTINATION "${CMAKE_SOURCE_DIR}/build/Release/")
else()
execute_process(COMMAND yarn
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/"
                OUTPUT_QUIET)
execute_process(COMMAND yarn build
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/"
                OUTPUT_QUIET)
file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/dist" 
    DESTINATION "${CMAKE_SOURCE_DIR}/build/")
endif()

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})
