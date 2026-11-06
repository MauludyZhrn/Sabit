cmake_minimum_required(VERSION 3.22)

project(Sabit
    VERSION 1.0.0
    DESCRIPTION "High-performance native Hilal live-view, processing, and detection application"
    LANGUAGES C CXX
)

# Use include_fix to route Qt array deallocations to Qt6Core.dll
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/include_fix)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencies
include(FetchContent)

find_package(spdlog QUIET)
find_package(nlohmann_json QUIET)
find_package(Qt6 COMPONENTS Widgets Core Gui Svg QUIET)

# Qlementine QStyle Engine (FetchContent with local vendor source resolution)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/qlementine/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_QLEMENTINE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/qlementine" CACHE PATH "Local Qlementine source")
endif()
FetchContent_Declare(
    qlementine
    GIT_REPOSITORY https://github.com/oclero/qlementine.git
    GIT_TAG main
)
if(Qt6Widgets_FOUND)
    FetchContent_MakeAvailable(qlementine)
endif()

# Subdirectories for modules
add_subdirectory(src/core)
add_subdirectory(src/camera)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/imaging/CMakeLists.txt")
    add_subdirectory(src/imaging)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/detect/CMakeLists.txt")
    add_subdirectory(src/detect)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/astro/CMakeLists.txt")
    add_subdirectory(src/astro)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/CMakeLists.txt")
    add_subdirectory(src/app)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/CMakeLists.txt")
    add_subdirectory(src/ui)
endif()

# Main Application Executable
if(TARGET sabit_ui)
    add_executable(Sabit
        src/main.cpp
    )
    target_link_libraries(Sabit PRIVATE
        sabit_ui
        sabit_app
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
    )
    target_compile_features(Sabit PRIVATE cxx_std_20)
endif()

# Testing
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
