/*
 * Composition root of the firmware for the Raspberry Pi Pico 2.
 *
 * This is the only place that knows both the hardware-independent code and
 * the Pico SDK. For now it starts the console, creates the device state
 * machine and reports its state.
 *
 * It drives one pin, the console output, which leads to the console header
 * and into no carrier circuit. Every other pin stays as the reset left it:
 * released, so that the resistors of the carrier hold every switch open,
 * keep the pre-regulator off and leave both converters of the slow bus
 * unselected (section 5 of the specification). The adapters for the
 * acquisition, the analog front end, the slow bus and USB are added with
 * the development phases that bring up the hardware; until then nothing
 * here may drive a line into the carrier.
 */
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "app/pp_fsm.h"
#include "base/pp_pins.h"
#include "base/pp_status.h"
#include "proto/pp_proto_defs.h"

#define CONSOLE_BAUD 115200u
#define IDLE_PERIOD_MS 1000u

static void log_transition(void *ctx, pp_device_state_t from, pp_device_state_t to,
                           pp_fsm_event_t event)
{
    (void)ctx;
    printf("state %s -> %s on %s\n", pp_fsm_state_name(from), pp_fsm_state_name(to),
           pp_fsm_event_name(event));
}

int main(void)
{
    static pp_fsm_t fsm;

    /* Console on UART0. The pad pull-up of its input is rule F-5. */
    stdio_uart_init_full(uart0, CONSOLE_BAUD, PP_GPIO_DBG_TX, PP_GPIO_DBG_RX);
    gpio_pull_up(PP_GPIO_DBG_RX);

    printf("power_profiler %s, protocol version %u\n", PP_FIRMWARE_VERSION,
           (unsigned)PP_PROTO_VERSION);

    pp_status_t status = pp_fsm_init(&fsm, log_transition, NULL);
    if (status != PP_OK) {
        printf("state machine init failed: %s\n", pp_status_name(status));
    } else {
        printf("state %s\n", pp_fsm_state_name(pp_fsm_state(&fsm)));
    }

    for (;;) {
        sleep_ms(IDLE_PERIOD_MS);
    }
}
