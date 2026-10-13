# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_SMU_SOURCES
    src/pp_dac.c
)

set(PP_SMU_REQUIRES base)
