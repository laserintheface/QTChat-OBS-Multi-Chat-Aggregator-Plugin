cmake_minimum_required(VERSION 3.16)

# Set project name and version
project(QTChat VERSION 1.0.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OBS paths - can be overridden via environment or command line
set(OBS_STUDIO_DIR "$ENV{OBS_STUDIO_DIR}" CACHE PATH "Path to OBS Studio build directory")
set(OBS_DEPS_DIR "$ENV{OBS_DEPS_DIR}" CACHE PATH "Path to OBS dependencies directory")
set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH};${CMAKE_PREFIX_PATH}" CACHE PATH "CMake prefix path for OBS")

# OBS output directory
set(OBS_OUTPUT_DIR "$ENV{OBS_OUTPUT_DIR}" CACHE PATH "OBS output directory")
if(NOT OBS_OUTPUT_DIR)
    set(OBS_OUTPUT_DIR "rundir")
endif()

# Find OBS - check multiple possible locations
find_package(libobs QUIET)

if(NOT libobs_FOUND)
    # Try OBS Studio build directory and CMAKE_PREFIX_PATH
    set(_OBS_SEARCH_PATHS "${OBS_STUDIO_DIR}/build" "${OBS_DEPS_DIR}" "${CMAKE_PREFIX_PATH}")
    find_package(libobs REQUIRED PATHS ${_OBS_SEARCH_PATHS})
endif()

# Find OBS frontend API
find_package(obs-frontend-api QUIET)

if(NOT obs-frontend-api_FOUND)
    set(_OBS_SEARCH_PATHS "${OBS_STUDIO_DIR}/build" "${OBS_DEPS_DIR}" "${CMAKE_PREFIX_PATH}")
    find_package(obs-frontend-api REQUIRED PATHS ${_OBS_SEARCH_PATHS})
endif()

# Find Qt6
find_package(Qt6 COMPONENTS Widgets Core Network QUIET)
if(NOT Qt6_FOUND)
    # Try Qt5 as fallback
    find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
    set(USE_QT5 ON)
endif()

# Plugin sources
set(PLUGIN_SOURCES
    src/plugin-main.cpp
    src/chat-dock.cpp
    src/chat-message.cpp
    src/twitch-irc.cpp
    src/youtube-chat.cpp
    src/rtmp-detector.cpp
    src/chat-manager.cpp
    src/settings-dialog.cpp
)

# Plugin headers
set(PLUGIN_HEADERS
    src/plugin-main.h
    src/chat-dock.h
    src/chat-message.h
    src/twitch-irc.h
    src/youtube-chat.h
    src/rtmp-detector.h
    src/chat-manager.h
    src/settings-dialog.h
)

# Add plugin library
add_library(${CMAKE_PROJECT_NAME} MODULE
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Find OBS include paths
if(OBS_STUDIO_DIR)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${OBS_STUDIO_DIR}/libobs"
        "${OBS_STUDIO_DIR}/UI/obs-frontend-api"
    )
endif()

if(OBS_DEPS_DIR)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${OBS_DEPS_DIR}/include"
    )
endif()

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    libobs
    obs-frontend-api
)

# Link Qt
if(USE_QT5)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        Qt5::Widgets
        Qt5::Network
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_DEPRECATED_WARNINGS)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
    )
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON)
endif()

# Output configuration
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${CMAKE_PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OBS_OUTPUT_DIR}/obs-plugins/64bit"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OBS_OUTPUT_DIR}/obs-plugins/64bit"
)

# Installation
install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION "obs-plugins/64bit")

# Windows-specific settings
if(WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".dll")
endif()
