cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

# Add OBS build paths to CMAKE_PREFIX_PATH so find_package can locate them
# The OBS build system installs to .deps/ directory
set(_OBS_VERSION "31.0.3")
list(APPEND CMAKE_PREFIX_PATH 
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/libobs"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/deps/w32-pthreads"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/UI/obs-frontend-api"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/plugins/plugin-support"
)

# Add library search path for linking
link_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/libobs"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/deps/w32-pthreads"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/build_x64/plugins/plugin-support"
)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core Network)
  if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
    set(USE_QT5 ON)
  endif()
  
  if(USE_QT5)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QT_DEPRECATED_WARNINGS)
  else()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON)
  endif()
endif()

# Create precompiled header
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h>
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  ./src/pch.h
  ./src/plugin-main.h
  ./src/plugin-main.cpp
  ./src/chat-dock.h
  ./src/chat-dock.cpp
  ./src/chat-message.h
  ./src/chat-message.cpp
  ./src/chat-manager.h
  ./src/chat-manager.cpp
  ./src/twitch-irc.h
  ./src/twitch-irc.cpp
  ./src/youtube-chat.h
  ./src/youtube-chat.cpp
  ./src/rtmp-detector.h
  ./src/rtmp-detector.cpp
  ./src/settings-dialog.h
  ./src/settings-dialog.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/libobs
  ${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-studio-${_OBS_VERSION}/plugins/plugin-support
  ${CMAKE_CURRENT_SOURCE_DIR}/.deps/include
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(WIN32)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".dll")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE user32.lib)
endif()
