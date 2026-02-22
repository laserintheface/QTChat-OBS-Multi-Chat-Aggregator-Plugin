# OBS CMake build number module
include_guard(GLOBAL)

set(_BUILD_NUMBER_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber" CACHE INTERNAL "OBS build number cache file")

if(NOT DEFINED OBS_BUILD_NUMBER)
  if(EXISTS "${_BUILD_NUMBER_CACHE}")
    file(READ "${_BUILD_NUMBER_CACHE}" OBS_BUILD_NUMBER)
    math(EXPR OBS_BUILD_NUMBER "${OBS_BUILD_NUMBER}+1")
  else()
    if("$ENV{CI}" AND "$ENV{GITHUB_RUN_ID}")
      set(OBS_BUILD_NUMBER "$ENV{GITHUB_RUN_ID}")
    else()
      set(OBS_BUILD_NUMBER "1")
    endif()
  endif()
  file(WRITE "${_BUILD_NUMBER_CACHE}" "${OBS_BUILD_NUMBER}")
endif()
