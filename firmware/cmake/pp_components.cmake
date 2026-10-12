# The hardware-independent components of the firmware, for both builds.
#
# The target build (firmware/CMakeLists.txt) and the unit test build
# (firmware/test/host/CMakeLists.txt) include this file, so both compile the
# same sources from the same manifests, components/<name>/sources.cmake.
#
# The including project sets PP_FIRMWARE_DIR to the firmware directory and has
# included pp_warnings.cmake. When it defines the interface library
# pp_build_flags, every component is built with the flags of that library.

# Components in the order of their dependencies.
set(PP_COMPONENTS base proto acq afe smu monitor app)

# Build components/<name> as the library pp_<name> from its manifest.
function(pp_add_component name)
    string(TOUPPER ${name} upper)
    set(dir ${PP_FIRMWARE_DIR}/components/${name})
    include(${dir}/sources.cmake)

    set(sources ${PP_${upper}_SOURCES})
    list(TRANSFORM sources PREPEND ${dir}/)
    set(requires ${PP_${upper}_REQUIRES})
    list(TRANSFORM requires PREPEND pp_)

    if(sources)
        add_library(pp_${name} STATIC ${sources})
        target_include_directories(pp_${name} PUBLIC ${dir}/include)
        target_link_libraries(pp_${name} PUBLIC ${requires})
        if(TARGET pp_build_flags)
            target_link_libraries(pp_${name} PRIVATE pp_build_flags)
        endif()
        pp_target_strict_warnings(pp_${name})
    else()
        add_library(pp_${name} INTERFACE)
        target_include_directories(pp_${name} INTERFACE ${dir}/include)
        target_link_libraries(pp_${name} INTERFACE ${requires})
    endif()
endfunction()

# Build every component of PP_COMPONENTS.
function(pp_add_components)
    foreach(name IN LISTS PP_COMPONENTS)
        pp_add_component(${name})
    endforeach()
endfunction()
