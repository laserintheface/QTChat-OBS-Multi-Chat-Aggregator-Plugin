# OBS CMake bootstrap module

include_guard(GLOBAL)

set(
  CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO
    RelWithDebInfo Release MinSizeRel None ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL
    MinSizeRel Release RelWithDebInfo None ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_RELEASE
    Release RelWithDebInfo MinSizeRel None ""
)

if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "In-source builds not supported.")
endif()

set(OBS_COMPANY_NAME "OBS Project")
set(OBS_PRODUCT_NAME "OBS Studio")
set(OBS_WEBSITE "https://www.obsproject.com")
set(OBS_CMAKE_VERSION 3.0.0)

set(_obs_default_version "0" "0" "1")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(policies)
include(versionconfig)
include(buildnumber)
include(osconfig)

if(NOT CMAKE_GENERATOR MATCHES "(Xcode|Visual Studio .+)")
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "OBS build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
  endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
