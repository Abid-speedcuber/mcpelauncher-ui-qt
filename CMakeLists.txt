cmake_minimum_required(VERSION 3.1)

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

set(MCPELAUNCHER_ROOT "/home/abid/mcpe" CACHE PATH "Path to the mcpelauncher source tree")
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

find_package(Qt6LinguistTools)
set(LAUNCHER_LINGUIST_RESOURCES "")
if (Qt6LinguistTools_FOUND)
    file(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ts")
    if(UPDATE_TRANSLATIONS)
        qt6_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
    else()
        qt6_add_translation(QM_FILES ${TS_FILES})
    endif()
    # Generate a .qrc file dynamically
    set(QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")

    file(WRITE  ${QRC_FILE} "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    file(APPEND ${QRC_FILE} "<RCC>\n")
    file(APPEND ${QRC_FILE} "  <qresource prefix=\"/translations\">\n")
    foreach(TS_FILE IN LISTS QM_FILES)
        # Extract just the filename (basename)
        get_filename_component(TS_BASENAME "${TS_FILE}" NAME)
        file(APPEND ${QRC_FILE} "    <file>${TS_BASENAME}</file>\n")
    endforeach()
    file(APPEND ${QRC_FILE} "  </qresource>\n")
    file(APPEND ${QRC_FILE} "</RCC>\n")
    set(LAUNCHER_LINGUIST_RESOURCES ${QRC_FILE} ${QM_FILES})
endif()

qt6_add_resources(RESOURCES qml.qrc)
set(RESOURCES ${RESOURCES} ${LAUNCHER_LINGUIST_RESOURCES})



add_executable(${PROJECT_NAME} main.cpp launcherapp.cpp launcherapp.h versionmanager.cpp versionmanager.h apkextractiontask.cpp apkextractiontask.h zipextractiontask.cpp zipextractiontask.h gamelauncher.cpp gamelauncher.h qmlurlutils.h launchersettings.h supportedandroidabis.cpp supportedandroidabis.h ${RESOURCES})

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
    install(FILES mcpelauncher-ui-qt.desktop COMPONENT mcpelauncher-ui-qt DESTINATION share/applications)
    install(FILES Resources/mcpelauncher-icon.svg RENAME mcpelauncher-ui-qt.svg
            COMPONENT mcpelauncher-ui-qt DESTINATION share/pixmaps)
endif()
include(CPackSettings.cmake)
