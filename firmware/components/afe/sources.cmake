# Manifest of the component, read by the target build and by the unit test
# build through cmake/pp_components.cmake. Paths are relative to this
# directory.
#
# The component has no hardware-independent source yet: it declares the port
# that the analog front end adapter will implement.

set(PP_AFE_SOURCES)

set(PP_AFE_REQUIRES base proto)
