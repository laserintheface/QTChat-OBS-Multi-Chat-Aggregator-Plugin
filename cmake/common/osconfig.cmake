# OBS CMake OS configuration module
include_guard(GLOBAL)

if(WIN32)
  set(OBS_OUTPUT_DIR "rundir" CACHE STRING "OBS output directory")
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(APPLE)
  set(OBS_OUTPUT_DIR "build" CACHE STRING "OBS output directory")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
else()
  set(OBS_OUTPUT_DIR "build" CACHE STRING "OBS output directory")
endif()
