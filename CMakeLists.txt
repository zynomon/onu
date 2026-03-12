cmake_minimum_required(VERSION 3.16)
project(onu VERSION 0.6 LANGUAGES CXX)

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
    $<$<CONFIG:Debug>:qrc/t.qrc>
)
target_compile_definitions(onu PRIVATE
    $<$<CONFIG:Debug>:CMAKE_DEBUG>
)

find_package(Trig REQUIRED)

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

# Install Onboarding.so to etc/skel for non-macOS Unix systems
if(UNIX AND NOT APPLE)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/dists/Onboarding.so
        DESTINATION etc/skel/.local/Onu/Conf/extensions
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()

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
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/dists/mac/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Onu"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# /*--------------------------
#
#                 Zynomon aelius <zynomon@proton.me> ©️2026, Apache 2.0
#                                                     project : Onu 0.6
#                                                 visit dists folder for deployment
#                                                            -----------------------*/

add_subdirectory(ext/Onboard)
