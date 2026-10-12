# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_BASE_SOURCES
    src/pp_blockq.c
    src/pp_pins.c
    src/pp_status.c
)

set(PP_BASE_REQUIRES)
