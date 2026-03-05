cmake_minimum_required(VERSION 3.16)

project(QrCode LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Core Svg Widgets WebEngineCore)
find_package(PkgConfig REQUIRED)
pkg_check_modules(QRENCODE REQUIRED libqrencode)

add_library(QrCode SHARED
    main.cxx
)

target_link_libraries(QrCode
    Trig::crash
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::Multimedia
    ${QRENCODE_LINK_LIBRARIES}
)

target_include_directories(QrCode PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${QRENCODE_INCLUDE_DIRS}
)

set_target_properties(QrCode PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
