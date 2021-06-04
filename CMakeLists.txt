cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 0.2.7 DESCRIPTION "A crossplatform soundboard")

set(FULL_VERSION_STRING "0.2.7")
option(EMBED_PATH "The path used for embedding" "OFF")
option(USE_FLATPAK "Allows the program to run under flatpak" OFF)

file(GLOB src
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    "src/*/*/*/*.cpp"
    "src/*/*/*/*/*.cpp"
)

if (WIN32)
    add_executable(soundux WIN32 ${src} "assets/icon.rc")

    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELWITHDEBINFO
    )

    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
        message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
    endforeach()

    target_compile_definitions(soundux PRIVATE WIN32_LEAN_AND_MEAN=1)
    target_compile_options(soundux PRIVATE /W4)
else()
    add_executable(soundux ${src})
    target_compile_options(soundux PRIVATE -Wall -Werror -Wextra -pedantic -Wno-unused-lambda-capture -Wno-gnu)
endif()

target_compile_definitions(soundux PRIVATE SOUNDUX_VERSION="${FULL_VERSION_STRING}" WNCK_I_KNOW_THIS_IS_UNSTABLE=1)
target_include_directories(soundux SYSTEM PRIVATE "src")
target_include_directories(soundux SYSTEM PRIVATE "lib/miniaudio")
target_include_directories(soundux SYSTEM PRIVATE "lib/semver/include")
target_include_directories(soundux SYSTEM PRIVATE "lib/fancypp/include")
target_include_directories(soundux SYSTEM PRIVATE "lib/json/single_include")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(soundux PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if (UNIX)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    find_package(PipeWire REQUIRED)

    find_package(PulseAudio)
    find_package(X11 REQUIRED)
    target_include_directories(soundux SYSTEM PRIVATE ${X11_INCLUDE_DIR} ${PULSEAUDIO_INCLUDE_DIR} ${PipeWire_INCLUDE_DIRS} ${Spa_INCLUDE_DIRS}) 

    find_package(PkgConfig REQUIRED)
	pkg_check_modules(WNCK libwnck-3.0)
    target_link_libraries(soundux PRIVATE ${X11_LIBRARIES} ${X11_Xinput_LIB} ${X11_XTest_LIB})
endif()
if (WIN32)
    target_compile_definitions(soundux PRIVATE _CRT_SECURE_NO_WARNINGS=1 _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _UNICODE=1)
endif()

add_subdirectory(src/ui/impl/webview/lib/webviewpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/nativefiledialog-extended EXCLUDE_FROM_ALL)
add_subdirectory(lib/tiny-process-library EXCLUDE_FROM_ALL)
add_subdirectory(lib/backward-cpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/traypp EXCLUDE_FROM_ALL)
add_subdirectory(lib/guardpp)
add_subdirectory(lib/lockpp)

add_backward(soundux)

if (WIN32)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(OPENSSL_MSVC_STATIC_RT  TRUE)
    set(BROTLI_USE_STATIC_LIBS TRUE)
endif()

set(HTTPLIB_REQUIRE_OPENSSL ON)
add_subdirectory(lib/cpp-httplib EXCLUDE_FROM_ALL)
target_include_directories(soundux SYSTEM PRIVATE "lib/cpp-httplib")

target_link_libraries(soundux PRIVATE webview nfd tiny-process-library tray guard httplib lockpp)

if (${EMBED_PATH} STREQUAL "OFF")
    message("Web-content will not be embedded") 
    
    # [[ Copy required files ]]
    #  > Copies frontend files
    if (MSVC)
        file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/" 
            DESTINATION "${CMAKE_SOURCE_DIR}/build/Release/dist")
    else()
        file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/" 
            DESTINATION "${CMAKE_SOURCE_DIR}/build/dist")
    endif()
else()
    message("Using embed path " ${EMBED_PATH})
    target_include_directories(webview PUBLIC ${EMBED_PATH})
    target_compile_definitions(soundux PRIVATE IS_EMBEDDED=1)
endif()

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES 
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF
                      CXX_STANDARD_REQUIRED ON)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})


if(USE_FLATPAK)
    target_compile_definitions(soundux PRIVATE USE_FLATPAK)
    target_link_libraries(soundux PRIVATE ${PULSEAUDIO_LIBRARY})

    install(TARGETS soundux DESTINATION bin)
    if (${EMBED_PATH} STREQUAL "OFF")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/build/dist" DESTINATION bin)
    endif()
else()
  set(CMAKE_INSTALL_PREFIX "/opt/soundux" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
  install(TARGETS soundux DESTINATION .)
  if (${EMBED_PATH} STREQUAL "OFF")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/build/dist" DESTINATION .)
  endif()
  install(FILES "${CMAKE_SOURCE_DIR}/deployment/soundux.desktop" DESTINATION /usr/share/applications)
  install(FILES "${CMAKE_SOURCE_DIR}/deployment/appstream/io.github.Soundux.metainfo.xml" DESTINATION /usr/share/metainfo)
  install(FILES "${CMAKE_SOURCE_DIR}/assets/soundux.png" DESTINATION /usr/share/pixmaps)
endif()
