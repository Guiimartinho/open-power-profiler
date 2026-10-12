# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.

set(PP_PROTO_SOURCES
    src/pp_command.c
    src/pp_crc16.c
    src/pp_frame.c
    src/pp_report.c
    src/pp_stream.c
)

set(PP_PROTO_REQUIRES base)
