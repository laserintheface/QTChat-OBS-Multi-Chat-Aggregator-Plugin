# OBS Plugin Helper Macros

# Find OBS Studio
find_package(obs-studio QUIET)

if(NOT OBS_STUDIO_FOUND)
    set(obs-studio_DIR "" CACHE PATH "Path to obs-studio build")
    find_package(obs-studio REQUIRED)
endif()

# Setup plugin info
macro(setup_plugin target)
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${target}"
        SUFFIX ".so"
    )

    if(WIN32)
        set_target_properties(${target} PROPERTIES
            SUFFIX ".dll"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/rundir"
        )
    elseif(APPLE)
        set_target_properties(${target} PROPERTIES
            SUFFIX ".dylib"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/rundir/${target}.plugin/Contents/MacOS"
        )
    endif()

    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/rundir/obs-plugins"
    )
endmacro()

# Install plugin
macro(install_plugin target)
    if(WIN32)
        install(TARGETS ${target} RUNTIME DESTINATION "obs-plugins")
    elseif(APPLE)
        install(TARGETS ${target} LIBRARY DESTINATION "${target}.plugin/Contents/MacOS")
    else()
        install(TARGETS ${target} LIBRARY DESTINATION "obs-plugins")
    endif()
endmacro()

# Compile Qt resources
macro(compile_resources name outvar)
    qt5_add_resources(${outvar} ${ARGN})
endmacro()
