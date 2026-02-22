# OBS CMake version helper module
include_guard(GLOBAL)

set(_obs_version ${_obs_default_version})
set(_obs_version_canonical ${_obs_default_version})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git describe --always --tags --dirty=-modified
    OUTPUT_VARIABLE _obs_version
    ERROR_VARIABLE _git_describe_err
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _obs_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(_obs_version_result EQUAL 0)
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _obs_version_canonical ${_obs_version})
  endif()
endif()

if(_obs_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-rc[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-rc([0-9]+).*$" "\\1" _obs_release_candidate ${_obs_version})
elseif(_obs_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-beta[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-beta([0-9]+).*$" "\\1" _obs_beta ${_obs_version})
endif()

list(GET _obs_version_canonical 0 OBS_VERSION_MAJOR)
list(GET _obs_version_canonical 1 OBS_VERSION_MINOR)
list(GET _obs_version_canonical 2 OBS_VERSION_PATCH)

set(OBS_RELEASE_CANDIDATE ${_obs_release_candidate})
set(OBS_BETA ${_obs_beta})

string(REPLACE ";" "." OBS_VERSION_CANONICAL "${_obs_version_canonical}")
string(REPLACE ";" "." OBS_VERSION "${_obs_version}")

unset(_obs_default_version)
unset(_obs_version)
unset(_obs_version_canonical)
unset(_obs_release_candidate)
unset(_obs_beta)
unset(_obs_version_result)
