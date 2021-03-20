cmake_minimum_required(VERSION 3.1)
project(soundux VERSION 0.2.2 DESCRIPTION "")

option(IS_CI_BUILD "Is used for windows ci build" OFF)
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
    target_compile_options(soundux PRIVATE -Wall -Wextra -Werror -pedantic -Wno-unused-lambda-capture)
endif()

target_compile_definitions(soundux PRIVATE SOUNDUX_VERSION="${PROJECT_VERSION}" WNCK_I_KNOW_THIS_IS_UNSTABLE=1)
target_include_directories(soundux SYSTEM PRIVATE "lib/miniaudio")
target_include_directories(soundux SYSTEM PRIVATE "lib/fancypp/include")
target_include_directories(soundux SYSTEM PRIVATE "lib/json/single_include")
target_include_directories(soundux SYSTEM PRIVATE "lib/InstanceGuard/Source")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(soundux PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if (UNIX)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    
    find_package(PkgConfig REQUIRED)
	pkg_check_modules(WNCK libwnck-3.0)
    target_link_libraries(soundux PRIVATE ${X11_LIBRARIES} ${X11_Xinput_LIB})
    
    if (${WNCK_FOUND})
        add_definitions(${WNCK_CFLAGS})
        target_link_libraries(soundux PRIVATE ${WNCK_LIBRARIES})
        target_include_directories(soundux PRIVATE ${WNCK_LIBRARY_DIRS})
        target_compile_definitions(soundux PRIVATE USE_WNCK=1)
    endif()

endif()
if (WIN32)
    target_compile_definitions(soundux PRIVATE _CRT_SECURE_NO_WARNINGS=1 _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1 _UNICODE=1)
endif()

add_subdirectory(src/ui/impl/webview/lib/webviewpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/nativefiledialog-extended EXCLUDE_FROM_ALL)
add_subdirectory(lib/tiny-process-library EXCLUDE_FROM_ALL)
add_subdirectory(lib/InstanceGuard/Source EXCLUDE_FROM_ALL)

set(HTTPLIB_REQUIRE_OPENSSL ON)
add_subdirectory(lib/cpp-httplib EXCLUDE_FROM_ALL)

target_link_libraries(soundux PUBLIC webview nfd InstanceGuard tiny-process-library httplib)

# [[ Copy Openssl dlls ]] - Needed because dlls are not copied to output folder in github action
if (MSVC)  
    if (IS_CI_BUILD)
        find_package(OpenSSL REQUIRED)
        file(COPY "${OPENSSL_INCLUDE_DIR}/../libcrypto-1_1-x64.dll"
             DESTINATION "${CMAKE_SOURCE_DIR}/build/Release")
        file(COPY "${OPENSSL_INCLUDE_DIR}/../libssl-1_1-x64.dll"
             DESTINATION "${CMAKE_SOURCE_DIR}/build/Release")
        message("Copied required dlls!")
    endif()
endif()

# [[ Build Frontend ]]
if (MSVC)
    file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/" 
        DESTINATION "${CMAKE_SOURCE_DIR}/build/Release/dist")
else()
    file(COPY "${CMAKE_SOURCE_DIR}/src/ui/impl/webview/lib/soundux-ui/" 
        DESTINATION "${CMAKE_SOURCE_DIR}/build/dist")
endif()

target_compile_features(soundux PRIVATE cxx_std_17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(soundux PROPERTIES CMAKE_CXX_EXTENSIONS Off)
set_target_properties(soundux PROPERTIES CMAKE_CXX_STANDARD_REQUIRED On)

set_target_properties(soundux PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(soundux PROPERTIES PROJECT_NAME ${PROJECT_NAME})

if(USE_FLATPAK)
  install(TARGETS soundux DESTINATION bin)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/build/dist" DESTINATION bin)
else()
  set(CMAKE_INSTALL_PREFIX "/opt/soundux" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
  install(TARGETS soundux DESTINATION .)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/build/dist" DESTINATION .)
  install(FILES "${CMAKE_SOURCE_DIR}/deployment/soundux.desktop" DESTINATION /usr/share/applications)
  install(FILES "${CMAKE_SOURCE_DIR}/deployment/appstream/io.github.Soundux.metainfo.xml" DESTINATION /usr/share/metainfo)
  install(FILES "${CMAKE_SOURCE_DIR}/assets/soundux.png" DESTINATION /usr/share/pixmaps)
endif()
