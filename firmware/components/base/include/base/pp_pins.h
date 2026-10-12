/*
 * The pin map of the controller (section 5 of the specification).
 *
 * This header is the one place in the firmware that holds a GPIO number. The
 * table behind it gives every pin its direction, its drive strength and its
 * pad pull, and the adapters configure the pads from it and from nothing
 * else. Rules F-4 and F-5 of section 6.6 are properties of the table, and a
 * unit test checks them.
 */
#ifndef PP_PINS_H
#define PP_PINS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GPIO numbers of the Raspberry Pi Pico 2. */
enum {
    PP_GPIO_DBG_TX = 0,
    PP_GPIO_DBG_RX = 1,
    PP_GPIO_GATE_R1 = 2,
    PP_GPIO_GATE_R2 = 3,
    PP_GPIO_GATE_R3 = 4,
    PP_GPIO_MUX_A0 = 5,
    PP_GPIO_MUX_A1 = 6,
    PP_GPIO_GATE_OUT = 7,
    PP_GPIO_CMP_UP = 8,
    PP_GPIO_CMP_JUMP = 9,
    PP_GPIO_CMP_OC = 10,
    PP_GPIO_VIN_OV = 11,
    PP_GPIO_SPI_MISO = 12,
    PP_GPIO_MON_CS = 13,
    PP_GPIO_SPI_SCK = 14,
    PP_GPIO_SPI_MOSI = 15,
    PP_GPIO_ADC_DOUT = 16,
    PP_GPIO_SIDE_DOUT = 17,
    PP_GPIO_SMU_ON = 18,
    PP_GPIO_ADC_SCK = 19,
    PP_GPIO_ADC_CNV = 20,
    PP_GPIO_SIDE_LOAD = 21,
    PP_GPIO_DAC_CS = 22,
    PP_GPIO_VBUS_SENSE = 24, /* on the module, not on the headers */
    PP_GPIO_LED = 25,        /* on the module, not on the headers */
    PP_GPIO_GATE_SRC = 26,
    PP_GPIO_GATE_AMP = 27,
    PP_GPIO_PWR_GOOD = 28,
};

typedef enum {
    PP_PIN_INPUT = 0,
    PP_PIN_OUTPUT,
} pp_pin_dir_t;

typedef enum {
    PP_PIN_PULL_NONE = 0,
    PP_PIN_PULL_UP,
    PP_PIN_PULL_DOWN, /* the state of a pad after reset */
} pp_pin_pull_t;

/* What drives or reads the pin. The adapter of that part configures it. */
typedef enum {
    PP_PIN_OWNER_CONSOLE = 0, /* UART of the console */
    PP_PIN_OWNER_SEQUENCER,   /* range sequencer: gates, address, comparators */
    PP_PIN_OWNER_OUTPUT,      /* output switch and over-current trip */
    PP_PIN_OWNER_CAPTURE,     /* capture of the converter and the side data */
    PP_PIN_OWNER_SLOW_BUS,    /* SPI to the DAC and the monitor converter */
    PP_PIN_OWNER_PATH,        /* mode requests and the enable of the source */
    PP_PIN_OWNER_STATUS,      /* status lines of the carrier and the module */
    PP_PIN_OWNER_INDICATOR,   /* LED of the module */
} pp_pin_owner_t;

typedef struct {
    uint8_t gpio;
    const char *signal;
    pp_pin_dir_t dir;
    pp_pin_owner_t owner;
    uint8_t drive_ma; /* drive strength of an output, zero for an input */
    pp_pin_pull_t pull;
    bool carrier; /* the pin is on the headers and connects to the carrier */
} pp_pin_t;

/* Number of entries in the table. */
size_t pp_pins_count(void);

/* Entry by position, or NULL past the end. */
const pp_pin_t *pp_pins_at(size_t index);

/* Entry of a GPIO, or NULL when the firmware does not use that GPIO. */
const pp_pin_t *pp_pins_find(uint8_t gpio);

/*
 * True when the table allows the firmware to drive the GPIO. Every function
 * that enables an output asks first: an input of the table, or a GPIO that
 * is not in it, is never driven (F-4).
 */
bool pp_pins_may_drive(uint8_t gpio);

/* One bit per GPIO for the pins toward the carrier with the given direction. */
uint32_t pp_pins_carrier_mask(pp_pin_dir_t dir);

/* One bit per GPIO for the pins that an owner has, inputs and outputs. */
uint32_t pp_pins_owner_mask(pp_pin_owner_t owner);

#endif /* PP_PINS_H */
