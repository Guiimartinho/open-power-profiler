# Warning flags of the hardware-independent code, for both builds.
#
# The components compile with the same strict set on the PC and for the
# target, and a warning is an error in both.

set(PP_STRICT_WARNINGS
    -Wall
    -Wextra
    -Wconversion
    -Wshadow
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wpedantic
    -Werror
)

function(pp_target_strict_warnings target)
    target_compile_options(${target} PRIVATE ${PP_STRICT_WARNINGS})
endfunction()
