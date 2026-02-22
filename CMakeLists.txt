cmake_minimum_required(VERSION 3.16)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_policy(SET CMP0048 NEW)
    project(QTChat VERSION 1.0.0)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

# OBS paths - can be overridden via environment or command line
set(OBS_SOURCE_DIR "$ENV{OBS_SOURCE_DIR}" CACHE PATH "Path to OBS Studio source directory")
set(OBS_DEPS_DIR "$ENV{OBS_DEPS_DIR}" CACHE PATH "Path to OBS dependencies directory")

# Try to find OBS if paths are provided
if(OBS_SOURCE_DIR)
    set(CMAKE_INCLUDE_PATH "${OBS_SOURCE_DIR}/libobs" "${CMAKE_INCLUDE_PATH}")
    set(CMAKE_INCLUDE_PATH "${OBS_SOURCE_DIR}/UI/obs-frontend-api" "${CMAKE_INCLUDE_PATH}")
    set(CMAKE_PREFIX_PATH "${OBS_DEPS_DIR}/lib/cmake" "${CMAKE_PREFIX_PATH}")
    
    # Find OBS libraries
    find_library(OBS_LIB obs-libobs REQUIRED)
    find_library(OBS_FRONTEND_API libobs-frontend-api REQUIRED)
    
    message(STATUS "Found OBS: ${OBS_LIB}")
endif()

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

set(QTCHAT_DATA
    data/locale/en-US.ini
)

compile_resources(QTCHAT_QRC qtchat.qrc ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${CMAKE_PROJECT_NAME} MODULE
    ${QTCHAT_SOURCES}
    ${QTCHAT_HEADERS}
    ${QTCHAT_QRC}
)

# Link OBS libraries if found, otherwise use find_package
if(OBS_LIB AND OBS_FRONTEND_API)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${OBS_SOURCE_DIR}/libobs"
        "${OBS_SOURCE_DIR}/UI/obs-frontend-api"
        "${OBS_DEPS_DIR}/include"
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        "${OBS_LIB}"
        "${OBS_FRONTEND_API}"
    )
else()
    # Try using OBS CMake targets as fallback
    find_package(obs-studio 30.0 QUIET)
    if(OBS_STUDIO_FOUND)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
            OBS::libobs
            OBS::obs-frontend-api
        )
    endif()
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Network
)

setup_plugin(${CMAKE_PROJECT_NAME} INFO)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    QT_DEPRECATED_WARNINGS
)

install_plugin(${CMAKE_PROJECT_NAME})
