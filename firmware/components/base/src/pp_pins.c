#include "base/pp_pins.h"

/* Drive strengths of rule F-5. */
#define DRIVE_GATE 12u /* lines into a gate driver behind a 1 kOhm pull-down */
#define DRIVE_STD 4u   /* every other output */

#define INPUT(number, name, who, pad_pull, on_carrier)                                             \
    {.gpio = (number),                                                                             \
     .signal = (name),                                                                             \
     .dir = PP_PIN_INPUT,                                                                          \
     .owner = (who),                                                                               \
     .drive_ma = 0u,                                                                               \
     .pull = (pad_pull),                                                                           \
     .carrier = (on_carrier)}

#define OUTPUT(number, name, who, drive, pad_pull, on_carrier)                                     \
    {.gpio = (number),                                                                             \
     .signal = (name),                                                                             \
     .dir = PP_PIN_OUTPUT,                                                                         \
     .owner = (who),                                                                               \
     .drive_ma = (drive),                                                                          \
     .pull = (pad_pull),                                                                           \
     .carrier = (on_carrier)}

/*
 * One line per pin, in the order of the table of section 5. A pad keeps the
 * pull-down it has after reset unless rule F-5 says otherwise: off on the two
 * chip selects and on PWR_GOOD, and a pull-up on the console input.
 */
static const pp_pin_t pins[] = {
    OUTPUT(PP_GPIO_DBG_TX, "DBG_TX", PP_PIN_OWNER_CONSOLE, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_DBG_RX, "DBG_RX", PP_PIN_OWNER_CONSOLE, PP_PIN_PULL_UP, true),
    OUTPUT(PP_GPIO_GATE_R1, "GATE_R1", PP_PIN_OWNER_SEQUENCER, DRIVE_GATE, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_GATE_R2, "GATE_R2", PP_PIN_OWNER_SEQUENCER, DRIVE_GATE, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_GATE_R3, "GATE_R3", PP_PIN_OWNER_SEQUENCER, DRIVE_GATE, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_MUX_A0, "MUX_A0", PP_PIN_OWNER_SEQUENCER, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_MUX_A1, "MUX_A1", PP_PIN_OWNER_SEQUENCER, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_GATE_OUT, "GATE_OUT", PP_PIN_OWNER_OUTPUT, DRIVE_GATE, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_CMP_UP, "CMP_UP", PP_PIN_OWNER_SEQUENCER, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_CMP_JUMP, "CMP_JUMP", PP_PIN_OWNER_SEQUENCER, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_CMP_OC, "CMP_OC", PP_PIN_OWNER_OUTPUT, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_VIN_OV, "VIN_OV", PP_PIN_OWNER_STATUS, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_SPI_MISO, "SPI_MISO", PP_PIN_OWNER_SLOW_BUS, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_MON_CS, "MON_CS", PP_PIN_OWNER_SLOW_BUS, DRIVE_STD, PP_PIN_PULL_NONE, true),
    OUTPUT(PP_GPIO_SPI_SCK, "SPI_SCK", PP_PIN_OWNER_SLOW_BUS, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_SPI_MOSI, "SPI_MOSI", PP_PIN_OWNER_SLOW_BUS, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_ADC_DOUT, "ADC_DOUT", PP_PIN_OWNER_CAPTURE, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_SIDE_DOUT, "SIDE_DOUT", PP_PIN_OWNER_CAPTURE, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_SMU_ON, "SMU_ON", PP_PIN_OWNER_PATH, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_ADC_SCK, "ADC_SCK", PP_PIN_OWNER_CAPTURE, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_ADC_CNV, "ADC_CNV", PP_PIN_OWNER_CAPTURE, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_SIDE_LOAD, "SIDE_LOAD", PP_PIN_OWNER_CAPTURE, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_DAC_CS, "DAC_CS", PP_PIN_OWNER_SLOW_BUS, DRIVE_STD, PP_PIN_PULL_NONE, true),
    INPUT(PP_GPIO_VBUS_SENSE, "VBUS_SENSE", PP_PIN_OWNER_STATUS, PP_PIN_PULL_DOWN, false),
    OUTPUT(PP_GPIO_LED, "LED", PP_PIN_OWNER_INDICATOR, DRIVE_STD, PP_PIN_PULL_DOWN, false),
    OUTPUT(PP_GPIO_GATE_SRC, "GATE_SRC", PP_PIN_OWNER_PATH, DRIVE_GATE, PP_PIN_PULL_DOWN, true),
    OUTPUT(PP_GPIO_GATE_AMP, "GATE_AMP", PP_PIN_OWNER_PATH, DRIVE_STD, PP_PIN_PULL_DOWN, true),
    INPUT(PP_GPIO_PWR_GOOD, "PWR_GOOD", PP_PIN_OWNER_STATUS, PP_PIN_PULL_NONE, true),
};

#define PIN_COUNT (sizeof(pins) / sizeof(pins[0]))

size_t pp_pins_count(void)
{
    return PIN_COUNT;
}

const pp_pin_t *pp_pins_at(size_t index)
{
    return (index < PIN_COUNT) ? &pins[index] : NULL;
}

const pp_pin_t *pp_pins_find(uint8_t gpio)
{
    for (size_t i = 0u; i < PIN_COUNT; i++) {
        if (pins[i].gpio == gpio) {
            return &pins[i];
        }
    }
    return NULL;
}

bool pp_pins_may_drive(uint8_t gpio)
{
    const pp_pin_t *pin = pp_pins_find(gpio);
    return (pin != NULL) && (pin->dir == PP_PIN_OUTPUT);
}

uint32_t pp_pins_carrier_mask(pp_pin_dir_t dir)
{
    uint32_t mask = 0u;
    for (size_t i = 0u; i < PIN_COUNT; i++) {
        if (pins[i].carrier && (pins[i].dir == dir)) {
            mask |= (uint32_t)1u << pins[i].gpio;
        }
    }
    return mask;
}

uint32_t pp_pins_owner_mask(pp_pin_owner_t owner)
{
    uint32_t mask = 0u;
    for (size_t i = 0u; i < PIN_COUNT; i++) {
        if (pins[i].owner == owner) {
            mask |= (uint32_t)1u << pins[i].gpio;
        }
    }
    return mask;
}
