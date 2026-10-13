# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_APP_SOURCES
    src/pp_fault.c
    src/pp_fsm.c
    src/pp_power_seq.c
    src/pp_sequence.c
)

set(PP_APP_REQUIRES base proto)
