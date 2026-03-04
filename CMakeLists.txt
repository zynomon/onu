cmake_minimum_required(VERSION 3.16)
project(onu VERSION 0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets WebEngineWidgets WebEngineCore Network Multimedia
)

add_executable(onu main.cxx
    qrc/resbin.qrc
    settings.Hxx
    extense.H
    Qtdl.H
    onu.H
    i.H
)

find_program(CMAKE_STRIP NAMES strip)
find_package(Trig)

target_link_libraries(onu PRIVATE
    Trig::crash
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::WebEngineWidgets
    Qt6::WebEngineCore Qt6::Network Qt6::Multimedia
)

if(CMAKE_STRIP)
    add_custom_command(TARGET onu POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:onu>
        COMMENT "Stripping onu binary"
    )
endif()

install(TARGETS onu
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/dists/onu.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/onu.desktop
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/onu.desktop
        DESTINATION share/applications
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dists/onu.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME onu.svg
    )
endif()

if(APPLE)
    set_target_properties(onu PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/dists/mac/Info.plist # Mistake;
        MACOSX_BUNDLE_BUNDLE_NAME "Onu"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()
# /*--------------------------
#
#                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
#                                                     project : Onu 0.5
#                                                 visit dists folder for deployment
#                                                            -----------------------*/
