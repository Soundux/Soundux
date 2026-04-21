cmake_minimum_required(VERSION 3.1)
project(webview VERSION 0.2 DESCRIPTION "A cross-platform C++ webview library")
option(WINDOWS_8 "Replaces the call to SetProcessDpiAwarenessContext call with SetProcessDpiAwareness to support Windows 8.1" OFF)

file(GLOB src
    "webview/src/*.cpp"
    "webview/src/*/*.cpp"
    "webview/src/*/*/*.cpp"
)

add_library(webview STATIC ${src})

if (WIN32)
    target_compile_options(webview PRIVATE /W4)
    
    find_program(NUGET_EXE NAMES nuget)
	if(NOT NUGET_EXE)
		message("NUGET.EXE not found.")
		message(FATAL_ERROR "Please install this executable, and run CMake again.")
	endif()

	exec_program(${NUGET_EXE}
    	ARGS install "Microsoft.Web.WebView2" -Version 1.0.865-prerelease -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages)
	exec_program(${NUGET_EXE}
    	ARGS install "Microsoft.Windows.ImplementationLibrary" -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages)

    if (WINDOWS_8)
        target_link_libraries(webview INTERFACE Shcore.lib)
        target_compile_definitions(webview PRIVATE WEBVIEWPP_WINDOWS_8=1)
    endif()

    target_link_libraries(webview INTERFACE Version.lib Shlwapi.lib ${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2/build/native/x64/WebView2LoaderStatic.lib ${CMAKE_BINARY_DIR}/packages/Microsoft.Windows.ImplementationLibrary/build/native/Microsoft.Windows.ImplementationLibrary.targets)
    target_include_directories(webview SYSTEM PUBLIC ${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2/build/native/include ${CMAKE_BINARY_DIR}/packages/Microsoft.Windows.ImplementationLibrary/include)
elseif(UNIX)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(webview PRIVATE -Wall -Wextra -Werror -pedantic -Wno-unused-lambda-capture)
    endif()

    find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.1)

    target_link_libraries(webview INTERFACE ${GTK3_LIBRARIES} ${WEBKIT2_LIBRARIES} pthread)
    target_include_directories(webview SYSTEM PUBLIC ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})
endif()

target_include_directories(webview SYSTEM PUBLIC "webview/include/")
target_include_directories(webview SYSTEM PUBLIC "lib/json/single_include/nlohmann")

target_compile_features(webview PRIVATE cxx_std_17)
set_target_properties(webview PROPERTIES 
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF
                      CXX_STANDARD_REQUIRED ON)

set_target_properties(webview PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(webview PROPERTIES PROJECT_NAME ${PROJECT_NAME})
