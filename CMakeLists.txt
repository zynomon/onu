cmake_minimum_required(VERSION 3.16)
project(onu VERSION 0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    WebEngineCore
    Network
    Multimedia
)

set(SOURCES
   onu.cpp
)

set(RESOURCES
    files.qrc
)

add_executable(onu
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(onu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::Multimedia
)

install(TARGETS onu
    RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_SOURCE_DIR}/dists/deb/onu.desktop.in
    DESTINATION share/applications
    RENAME onu.desktop
)

install(FILES ${CMAKE_SOURCE_DIR}/dists/deb/onu.svg
    DESTINATION share/icons/hicolor/scalable/apps
)

target_compile_options(onu PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

set_target_properties(onu PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_custom_target(run
    COMMAND onu
    DEPENDS onu
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "onu")
set(CPACK_PACKAGE_VENDOR "error.os")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Onu is a lightweight Qt-based browser/editor hybrid with WebEngine integration. \
    It supports HTML rendering, multimedia playback, and system integration with minimal footprint. \
    Designed for clarity, performance, and survivability in error.os environments."
)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_PACKAGE_CONTACT "Zynomon Aelius <zynomon@proton.me>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zynomon Aelius")

set(CPACK_GENERATOR "DEB;TGZ;ZIP")

set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6webenginewidgets6, libqt6webenginecore6, libqt6network6, libqt6multimedia6"
)

include(CPack)
