# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_MONITOR_SOURCES
    src/pp_monitor.c
)

set(PP_MONITOR_REQUIRES base proto)
