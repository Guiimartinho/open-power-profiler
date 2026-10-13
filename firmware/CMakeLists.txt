# Firmware of the instrument for the Raspberry Pi Pico 2 (RP2350).
#
# A project of the Pico SDK. The SDK is found through PICO_SDK_PATH and the
# compiler through PICO_TOOLCHAIN_PATH, from the environment or from the
# command line; README.md has the commands. The hardware-independent
# components are the ones the unit tests build, from the same manifests.
cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD pico2 CACHE STRING "Board of the instrument: the Raspberry Pi Pico 2")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "RP2350 with its Arm cores")

# Must come before project().
include(cmake/pico_sdk_import.cmake)

project(power_profiler VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

set(PP_FIRMWARE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include(cmake/pp_warnings.cmake)
include(cmake/pp_components.cmake)

pp_add_components()

# The composition root: the only code that includes headers of the SDK.
add_executable(power_profiler main/main.c)

target_compile_definitions(power_profiler PRIVATE
    PP_FIRMWARE_VERSION="${PROJECT_VERSION}"
)
target_compile_options(power_profiler PRIVATE -Wall -Wextra -Werror)

# It names every component, so each one is compiled and linked for the target.
set(pp_libraries ${PP_COMPONENTS})
list(TRANSFORM pp_libraries PREPEND pp_)
target_link_libraries(power_profiler PRIVATE ${pp_libraries} pico_stdlib)

# The console is UART0 on GP0 and GP1 (section 5 of the specification). USB
# belongs to the host protocol and is not a console.
pico_enable_stdio_uart(power_profiler 1)
pico_enable_stdio_usb(power_profiler 0)

pico_set_program_name(power_profiler "Open Power Profiler")
pico_set_program_version(power_profiler "${PROJECT_VERSION}")

# The .uf2 image, with the .bin, .hex and .map files beside it.
pico_add_extra_outputs(power_profiler)
