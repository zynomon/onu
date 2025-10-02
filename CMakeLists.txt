cmake_minimum_required(VERSION 3.18)
project(Onu VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineCore
    WebEngineWidgets
    Network
    Multimedia
)

# Qt build features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Executable
add_executable(Onu
    main.cpp
    resources.qrc
    onu.svgz
)

# ✅ Link WebEngineCore explicitly
target_link_libraries(Onu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::Network
    Qt6::Multimedia
)

# Output properties
set_target_properties(Onu PROPERTIES
    OUTPUT_NAME "Onu"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Desktop entry (Linux only)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/onu.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/onu.desktop
    @ONLY
)

# Install binary
install(TARGETS Onu
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

# Install desktop entry
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/onu.desktop
    DESTINATION share/applications
    COMPONENT desktop
)

# Install icon (SVGZ or SVG — pick one or both)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/onu.svgz
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT icons
)

# --- CPack Packaging ---
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "Onu")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Zynomon Aelius")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Linux: DEB
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zynomon Aelius")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Onu: a fast browser")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

# Windows: EXE via NSIS or ZIP
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")

    # NSIS Branding
    set(CPACK_NSIS_DISPLAY_NAME "Onu — a blazing fast browser")
    set(CPACK_NSIS_PACKAGE_NAME "Onu")
    set(CPACK_NSIS_CONTACT "Zynomon Aelius")
    set(CPACK_NSIS_MODIFY_PATH ON)

    # Start menu shortcut
    set(CPACK_NSIS_MENU_LINKS
        "bin/Onu.exe" "Onu Browser"
    )

    # Desktop shortcut
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut \\\"$DESKTOP\\Onu.lnk\\\" \\\"$INSTDIR\\\\bin\\\\Onu.exe\\\""
    )

    # Uninstall shortcut cleanup
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete \\\"$DESKTOP\\Onu.lnk\\\""
    )
endif()

set(CPACK_COMPONENTS_ALL runtime desktop icons)

include(CPack)
