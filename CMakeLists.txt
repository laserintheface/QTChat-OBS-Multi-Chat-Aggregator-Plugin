cmake_minimum_required(VERSION 3.16)

# Set project name and version
project(QTChat VERSION 1.0.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OBS paths from environment or defaults
set(OBS_STUDIO_DIR "$ENV{OBS_STUDIO_DIR}" CACHE PATH "Path to OBS Studio source")
set(OBS_DEPS_DIR "$ENV{OBS_DEPS_DIR}" CACHE PATH "Path to OBS dependencies")
set(OBS_OUTPUT_DIR "$ENV{OBS_OUTPUT_DIR}" CACHE PATH "OBS output directory")

if(NOT OBS_OUTPUT_DIR)
  set(OBS_OUTPUT_DIR "rundir")
endif()

# Find Qt6 (OBS 30+ uses Qt6)
find_package(Qt6 COMPONENTS Widgets Core Network QUIET)

if(NOT Qt6_FOUND)
  # Fallback to Qt5
  find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
  set(USE_QT5 ON)
endif()

# Find libobs
find_path(LIBOBS_INCLUDE_DIR NAMES obs-module.h PATHS "${OBS_STUDIO_DIR}/libobs" "${OBS_DEPS_DIR}/include" REQUIRED)
find_library(LIBOBS_LIBRARY NAMES obs libobs PATHS "${OBS_DEPS_DIR}/lib" REQUIRED)

# Find obs-frontend-api
find_path(OBS_FRONTEND_INCLUDE_DIR NAMES obs-frontend-api.h PATHS "${OBS_STUDIO_DIR}/UI/obs-frontend-api" "${OBS_DEPS_DIR}/include" REQUIRED)
find_library(OBS_FRONTEND_LIBRARY NAMES obs-frontend-api libobs-frontend-api PATHS "${OBS_DEPS_DIR}/lib" REQUIRED)

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

# Create plugin library
add_library(${CMAKE_PROJECT_NAME} MODULE
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBOBS_INCLUDE_DIR}
    ${OBS_FRONTEND_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    ${LIBOBS_LIBRARY}
    ${OBS_FRONTEND_LIBRARY}
)

# Link Qt
if(USE_QT5)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_DEPRECATED_WARNINGS)
else()
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON)
endif()

# Windows-specific settings
if(WIN32)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
      PREFIX ""
      OUTPUT_NAME "${CMAKE_PROJECT_NAME}"
      SUFFIX ".dll"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OBS_OUTPUT_DIR}/obs-plugins/64bit"
  )
else()
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
      PREFIX ""
      OUTPUT_NAME "${CMAKE_PROJECT_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OBS_OUTPUT_DIR}/obs-plugins"
  )
endif()

# Installation
install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION "obs-plugins/64bit")
