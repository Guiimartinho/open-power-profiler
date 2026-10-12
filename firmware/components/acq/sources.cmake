# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_ACQ_SOURCES
    src/pp_downrange.c
    src/pp_range_tracker.c
    src/pp_underrange.c
)

set(PP_ACQ_REQUIRES base proto)
