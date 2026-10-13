/*
 * The pin table against section 5 of the specification and rules F-4 and
 * F-5 of section 6.6.
 */
#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "base/pp_pins.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t bit(unsigned gpio)
{
    return (uint32_t)1u << gpio;
}

/* GP0 to GP22 and GP26 to GP28: the GPIO pins on the headers of the module. */
static bool on_headers(unsigned gpio)
{
    return (gpio <= 22u) || ((gpio >= 26u) && (gpio <= 28u));
}

static void test_pins_every_header_pin_is_in_the_table_once(void)
{
    unsigned seen[32] = {0};
    for (size_t i = 0u; i < pp_pins_count(); i++) {
        const pp_pin_t *pin = pp_pins_at(i);
        TEST_ASSERT_NOT_NULL(pin);
        TEST_ASSERT_LESS_THAN_UINT8(32u, pin->gpio);
        seen[pin->gpio]++;
    }
    for (unsigned gpio = 0u; gpio < 32u; gpio++) {
        if (on_headers(gpio)) {
            TEST_ASSERT_EQUAL_UINT_MESSAGE(1u, seen[gpio], "header pin missing or listed twice");
        } else {
            TEST_ASSERT_LESS_OR_EQUAL_UINT_MESSAGE(1u, seen[gpio], "pin listed twice");
        }
    }
}

static void test_pins_carrier_flag_matches_the_headers(void)
{
    for (size_t i = 0u; i < pp_pins_count(); i++) {
        const pp_pin_t *pin = pp_pins_at(i);
        TEST_ASSERT_EQUAL_MESSAGE(on_headers(pin->gpio), pin->carrier, pin->signal);
    }
}

/* F-4: these are driven by the carrier and are never outputs. */
static void test_pins_f4_carrier_outputs_are_never_driven(void)
{
    static const uint8_t never_outputs[] = {1u, 8u, 9u, 10u, 11u, 12u, 16u, 17u, 28u};
    uint32_t expected = 0u;
    for (size_t i = 0u; i < sizeof(never_outputs); i++) {
        const pp_pin_t *pin = pp_pins_find(never_outputs[i]);
        TEST_ASSERT_NOT_NULL(pin);
        TEST_ASSERT_EQUAL_MESSAGE(PP_PIN_INPUT, pin->dir, pin->signal);
        TEST_ASSERT_FALSE_MESSAGE(pp_pins_may_drive(never_outputs[i]), pin->signal);
        expected |= bit(never_outputs[i]);
    }
    /* They are all the inputs toward the carrier, and nothing else is one. */
    TEST_ASSERT_EQUAL_HEX32(expected, pp_pins_carrier_mask(PP_PIN_INPUT));
}

static void test_pins_inputs_and_outputs_share_no_pin(void)
{
    uint32_t inputs = pp_pins_carrier_mask(PP_PIN_INPUT);
    uint32_t outputs = pp_pins_carrier_mask(PP_PIN_OUTPUT);
    TEST_ASSERT_EQUAL_HEX32(0u, inputs & outputs);
    /* Every GPIO of the headers is in use (section 5). */
    TEST_ASSERT_EQUAL_HEX32(0x1C7FFFFFu, inputs | outputs);
}

static void test_pins_a_pin_outside_the_table_is_never_driven(void)
{
    TEST_ASSERT_NULL(pp_pins_find(23u));
    TEST_ASSERT_NULL(pp_pins_find(29u));
    TEST_ASSERT_NULL(pp_pins_find(200u));
    TEST_ASSERT_FALSE(pp_pins_may_drive(23u));
    TEST_ASSERT_FALSE(pp_pins_may_drive(29u));
    TEST_ASSERT_FALSE(pp_pins_may_drive(200u));
    TEST_ASSERT_NULL(pp_pins_at(pp_pins_count()));
}

/* F-5: 12 mA on GP2, GP3, GP4, GP7 and GP26, 4 mA on every other output. */
static void test_pins_f5_drive_strengths(void)
{
    const uint32_t strong = bit(2u) | bit(3u) | bit(4u) | bit(7u) | bit(26u);
    for (size_t i = 0u; i < pp_pins_count(); i++) {
        const pp_pin_t *pin = pp_pins_at(i);
        if (pin->dir == PP_PIN_INPUT) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0u, pin->drive_ma, pin->signal);
        } else if ((strong & bit(pin->gpio)) != 0u) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(12u, pin->drive_ma, pin->signal);
        } else {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(4u, pin->drive_ma, pin->signal);
        }
    }
}

/* F-5: pull-downs off on GP13, GP22 and GP28, a pull-up on GP1. */
static void test_pins_f5_pad_pulls(void)
{
    for (size_t i = 0u; i < pp_pins_count(); i++) {
        const pp_pin_t *pin = pp_pins_at(i);
        pp_pin_pull_t expected = PP_PIN_PULL_DOWN;
        if ((pin->gpio == 13u) || (pin->gpio == 22u) || (pin->gpio == 28u)) {
            expected = PP_PIN_PULL_NONE;
        } else if (pin->gpio == 1u) {
            expected = PP_PIN_PULL_UP;
        }
        TEST_ASSERT_EQUAL_MESSAGE(expected, pin->pull, pin->signal);
    }
}

/* Section 5: the pins of a state machine are runs of consecutive numbers. */
static void test_pins_groups_of_the_state_machines(void)
{
    /* Range gates and address GP2 to GP6, comparators GP8 and GP9. */
    TEST_ASSERT_EQUAL_HEX32(0x0000037Cu, pp_pins_owner_mask(PP_PIN_OWNER_SEQUENCER));
    /* Output switch GP7 and the over-current comparator GP10. */
    TEST_ASSERT_EQUAL_HEX32(bit(7u) | bit(10u), pp_pins_owner_mask(PP_PIN_OWNER_OUTPUT));
    /* Data pins GP16 and GP17, clock, convert-start and load GP19 to GP21. */
    TEST_ASSERT_EQUAL_HEX32(0x003B0000u, pp_pins_owner_mask(PP_PIN_OWNER_CAPTURE));
    /* SPI1 on GP12, GP14 and GP15 with the chip selects GP13 and GP22. */
    TEST_ASSERT_EQUAL_HEX32(0x0040F000u, pp_pins_owner_mask(PP_PIN_OWNER_SLOW_BUS));
    TEST_ASSERT_EQUAL_HEX32(bit(18u) | bit(26u) | bit(27u), pp_pins_owner_mask(PP_PIN_OWNER_PATH));
    TEST_ASSERT_EQUAL_HEX32(bit(11u) | bit(24u) | bit(28u),
                            pp_pins_owner_mask(PP_PIN_OWNER_STATUS));
    TEST_ASSERT_EQUAL_HEX32(bit(0u) | bit(1u), pp_pins_owner_mask(PP_PIN_OWNER_CONSOLE));
    TEST_ASSERT_EQUAL_HEX32(bit(25u), pp_pins_owner_mask(PP_PIN_OWNER_INDICATOR));
}

static void test_pins_signals_match_the_pin_map(void)
{
    TEST_ASSERT_EQUAL_STRING("GATE_R3", pp_pins_find(PP_GPIO_GATE_R3)->signal);
    TEST_ASSERT_EQUAL_STRING("GATE_OUT", pp_pins_find(PP_GPIO_GATE_OUT)->signal);
    TEST_ASSERT_EQUAL_STRING("CMP_JUMP", pp_pins_find(PP_GPIO_CMP_JUMP)->signal);
    TEST_ASSERT_EQUAL_STRING("PWR_GOOD", pp_pins_find(PP_GPIO_PWR_GOOD)->signal);
    TEST_ASSERT_EQUAL_UINT8(9u, PP_GPIO_CMP_JUMP);
    TEST_ASSERT_EQUAL_UINT8(28u, PP_GPIO_PWR_GOOD);
    TEST_ASSERT_TRUE(pp_pins_may_drive(PP_GPIO_GATE_OUT));
    TEST_ASSERT_TRUE(pp_pins_may_drive(PP_GPIO_SMU_ON));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pins_every_header_pin_is_in_the_table_once);
    RUN_TEST(test_pins_carrier_flag_matches_the_headers);
    RUN_TEST(test_pins_f4_carrier_outputs_are_never_driven);
    RUN_TEST(test_pins_inputs_and_outputs_share_no_pin);
    RUN_TEST(test_pins_a_pin_outside_the_table_is_never_driven);
    RUN_TEST(test_pins_f5_drive_strengths);
    RUN_TEST(test_pins_f5_pad_pulls);
    RUN_TEST(test_pins_groups_of_the_state_machines);
    RUN_TEST(test_pins_signals_match_the_pin_map);
    return UNITY_END();
}
