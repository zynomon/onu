cmake_minimum_required(VERSION 3.16)
project(onu VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineCore
    WebEngineWidgets
    Network
)

set(SOURCES
    main.cpp
    resources.qrc
)

add_executable(onu ${SOURCES})

target_link_libraries(onu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::Network
)

set_target_properties(onu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS onu
    RUNTIME DESTINATION bin
)

install(FILES resources/onu.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME onu.svg
)

configure_file(
    resources/onu.desktop.in
    ${CMAKE_BINARY_DIR}/onu.desktop
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/onu.desktop
    DESTINATION share/applications
)

set(CPACK_PACKAGE_NAME "onu")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Zynomon Aelius <zynomon@proton.me>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zynomon Aelius")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "onu — a Qt6 WebEngine-based application")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base-dev, qt6-webengine-dev")
set(CPACK_GENERATOR "DEB")

include(CPack)
