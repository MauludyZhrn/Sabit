cmake_minimum_required(VERSION 3.22)
project(qlementine VERSION 1.2.0 LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)

set(CMAKE_AUTOMOC ON)

add_library(qlementine STATIC
    include/oclero/qlementine/style/Theme.hpp
    src/style/Theme.cpp
    include/oclero/qlementine/animation/WidgetAnimator.hpp
    src/animation/WidgetAnimator.cpp
    include/oclero/qlementine/style/QlementineStyle.hpp
    src/style/QlementineStyle.cpp
)

add_library(oclero::qlementine ALIAS qlementine)

target_include_directories(qlementine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(qlementine PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

target_compile_features(qlementine PUBLIC cxx_std_20)
