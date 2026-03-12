cmake_minimum_required(VERSION 3.16)

project(Onboarding LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebEngineCore)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(Onboarding SHARED
    main.cxx
)

target_link_libraries(Onboarding
    Trig::crash
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::Multimedia
)

set_target_properties(Onboarding PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../dists
)