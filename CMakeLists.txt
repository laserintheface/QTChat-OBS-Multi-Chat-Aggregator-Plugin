cmake_minimum_required(VERSION 3.16)
project(QTChat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_policy(SET CMP0048 NEW)
    project(QTChat VERSION 1.0.0)
endif()

find_package(obs-studio 30.0 QUIET)
if(NOT OBS_STUDIO_FOUND)
    message(WARNING "obs-studio not found, using FindPackage mode search")
    find_package(obs-studio REQUIRED)
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ObsPluginHelpers.cmake)

set(QTCHAT_SOURCES
    src/plugin-main.cpp
    src/plugin-main.h
    src/chat-dock.h
    src/chat-dock.cpp
    src/chat-message.h
    src/chat-message.cpp
    src/twitch-irc.h
    src/twitch-irc.cpp
    src/youtube-chat.h
    src/youtube-chat.cpp
    src/rtmp-detector.h
    src/rtmp-detector.cpp
    src/chat-manager.h
    src/chat-manager.cpp
    src/settings-dialog.h
    src/settings-dialog.cpp
)

set(QTCHAT_HEADERS
    src/plugin-main.h
    src/chat-dock.h
    src/chat-message.h
    src/twitch-irc.h
    src/youtube-chat.h
    src/rtmp-detector.h
    src/chat-manager.h
    src/settings-dialog.h
)

set(QTCHAT_UI
    ui/settings-dialog.ui
)

set(QTCHAT_DATA
    data/locale/en-US.ini
)

compile_resources(QTCHAT_QRC qtchat.qrc ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${CMAKE_PROJECT_NAME} MODULE
    ${QTCHAT_SOURCES}
    ${QTCHAT_HEADERS}
    ${QTCHAT_UI}
    ${QTCHAT_QRC}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt5::Widgets
    Qt5::Network
    pthread
)

setup_plugin(${CMAKE_PROJECT_NAME} INFO)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    QT_DEPRECATED_WARNINGS
)

install_plugin(${CMAKE_PROJECT_NAME})
