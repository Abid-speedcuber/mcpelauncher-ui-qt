cmake_minimum_required(VERSION 3.16)

project(mcpelauncher-ui-qt LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# protobuf now requires c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Quick Concurrent Svg Widgets REQUIRED)
find_package(ZLIB REQUIRED)

if(NOT TARGET zlib)
    add_library(zlib ALIAS ZLIB::ZLIB)
endif()

set(MCPELAUNCHER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party" CACHE PATH "Path to the mcpelauncher dependency source tree")
if(EXISTS "${MCPELAUNCHER_ROOT}/axml-parser/CMakeLists.txt" AND NOT TARGET axml_parser)
    add_subdirectory("${MCPELAUNCHER_ROOT}/axml-parser" "${CMAKE_CURRENT_BINARY_DIR}/deps/axml-parser" EXCLUDE_FROM_ALL)
endif()
if(EXISTS "${MCPELAUNCHER_ROOT}/mcpelauncher-extract/CMakeLists.txt" AND NOT TARGET mcpelauncher-extract-lib)
    add_subdirectory("${MCPELAUNCHER_ROOT}/mcpelauncher-extract" "${CMAKE_CURRENT_BINARY_DIR}/deps/mcpelauncher-extract" EXCLUDE_FROM_ALL)
endif()
if(EXISTS "${MCPELAUNCHER_ROOT}/mcpelauncher-common/CMakeLists.txt" AND NOT TARGET mcpelauncher-common)
    add_subdirectory("${MCPELAUNCHER_ROOT}/mcpelauncher-common" "${CMAKE_CURRENT_BINARY_DIR}/deps/mcpelauncher-common" EXCLUDE_FROM_ALL)
endif()
if(EXISTS "${MCPELAUNCHER_ROOT}/mcpelauncher-apkinfo/CMakeLists.txt" AND NOT TARGET mcpelauncher-apkinfo)
    add_subdirectory("${MCPELAUNCHER_ROOT}/mcpelauncher-apkinfo" "${CMAKE_CURRENT_BINARY_DIR}/deps/mcpelauncher-apkinfo" EXCLUDE_FROM_ALL)
endif()
if(EXISTS "${MCPELAUNCHER_ROOT}/file-util/CMakeLists.txt" AND NOT TARGET file-util)
    add_subdirectory("${MCPELAUNCHER_ROOT}/file-util" "${CMAKE_CURRENT_BINARY_DIR}/deps/file-util" EXCLUDE_FROM_ALL)
endif()

option(LAUNCHER_DISABLE_DEV_MODE "Disables the dev tab by default" ON)
option(LAUNCHER_MACOS_HAVE_ARMLAUNCHER "Enables use of the arm launcher" ON)

qt6_add_resources(RESOURCES qml.qrc)

add_executable(${PROJECT_NAME} main.cpp launcherapp.cpp launcherapp.h versionmanager.cpp versionmanager.h apkextractiontask.cpp apkextractiontask.h zipextractiontask.cpp zipextractiontask.h packimporttask.cpp packimporttask.h gamelauncher.cpp gamelauncher.h qmlurlutils.h launchersettings.h supportedandroidabis.cpp supportedandroidabis.h ${RESOURCES})

if(LAUNCHER_DISABLE_DEV_MODE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC LAUNCHER_DISABLE_DEV_MODE)
endif()
if(LAUNCHER_MACOS_HAVE_ARMLAUNCHER)
    target_compile_definitions(${PROJECT_NAME} PUBLIC LAUNCHER_MACOS_HAVE_ARMLAUNCHER)
endif()

if (APPLE)
    target_sources(${PROJECT_NAME} PUBLIC launcherapp_osx.mm supportedandroidabis.mm)
    target_link_libraries(${PROJECT_NAME} "-framework AppKit")
endif()

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Quick Qt6::Widgets Qt6::Svg Qt6::Concurrent mcpelauncher-extract-lib mcpelauncher-common mcpelauncher-apkinfo file-util zlib)

install(TARGETS mcpelauncher-ui-qt RUNTIME COMPONENT mcpelauncher-ui-qt DESTINATION bin)
if (NOT APPLE)
    install(FILES mcpelauncher-ui-qt.desktop RENAME io.github.Abid_speedcuber.mcpelauncher_minimal.desktop
            COMPONENT mcpelauncher-ui-qt DESTINATION share/applications)
    install(FILES packaging/io.github.Abid_speedcuber.mcpelauncher_minimal.metainfo.xml
            COMPONENT mcpelauncher-ui-qt DESTINATION share/metainfo)
    install(FILES Resources/mcpelauncher-icon.svg RENAME io.github.Abid_speedcuber.mcpelauncher_minimal.svg
            COMPONENT mcpelauncher-ui-qt DESTINATION share/icons/hicolor/scalable/apps)
endif()
include(CPackSettings.cmake)
