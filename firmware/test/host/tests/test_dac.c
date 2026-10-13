/*
 * The set-point DAC against rules F-6 and F-30 of section 6.6 and the
 * nominal codes of section 4.2 of the specification.
 */
#include "unity.h"

#include <stdint.h>

#include "smu/pp_dac.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* F-6: the frame is 0x5000 | code. */
static void test_dac_frame_is_the_command_nibble_and_the_code(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x5000u, pp_dac_frame(0u));
    TEST_ASSERT_EQUAL_HEX16(0x5001u, pp_dac_frame(1u));
    TEST_ASSERT_EQUAL_HEX16(0x5268u, pp_dac_frame(616u));
    TEST_ASSERT_EQUAL_HEX16(0x5FFFu, pp_dac_frame(4095u));
    for (uint16_t code = 0u; code <= PP_DAC_CODE_MAX; code++) {
        TEST_ASSERT_EQUAL_HEX16((uint16_t)(0x5000u | code), pp_dac_frame(code));
    }
}

/* F-6: no other upper nibble can be sent, whatever the caller passes. */
static void test_dac_frame_never_sends_another_command_nibble(void)
{
    for (uint32_t value = 0u; value <= 0xFFFFu; value++) {
        uint16_t frame = pp_dac_frame((uint16_t)value);
        TEST_ASSERT_EQUAL_HEX16(0x5000u, frame & 0xF000u);
    }
}

static void test_dac_frame_limits_a_code_that_is_too_large(void)
{
    TEST_ASSERT_EQUAL_HEX16(pp_dac_frame(4095u), pp_dac_frame(4096u));
    TEST_ASSERT_EQUAL_HEX16(pp_dac_frame(4095u), pp_dac_frame(0xFFFFu));
}

/* Section 4.2: 30 mV to 5.26 V in steps of 1.28 mV; code zero is the offset. */
static void test_dac_nominal_output_of_a_code(void)
{
    TEST_ASSERT_EQUAL_UINT32(10000u, pp_dac_nominal_microvolts(0u));
    TEST_ASSERT_EQUAL_UINT32(11282u, pp_dac_nominal_microvolts(1u));
    TEST_ASSERT_EQUAL_UINT32(5258562u, pp_dac_nominal_microvolts(4095u));
    TEST_ASSERT_EQUAL_UINT32(5258562u, pp_dac_nominal_microvolts(0xFFFFu));
}

/* Section 4.2: nominal codes 616 for 0.80 V and 3893 for 5.00 V. */
static void test_dac_nominal_codes_of_the_two_limits(void)
{
    uint16_t code = 0u;
    TEST_ASSERT_EQUAL(PP_OK, pp_dac_nominal_code(800u, &code));
    TEST_ASSERT_EQUAL_UINT16(616u, code);
    TEST_ASSERT_EQUAL(PP_OK, pp_dac_nominal_code(5000u, &code));
    TEST_ASSERT_EQUAL_UINT16(3893u, code);
}

/* F-30: SET_VOLTAGE is limited to 0.80 V to 5.00 V. */
static void test_dac_nominal_code_refuses_a_set_point_outside_the_limits(void)
{
    static const uint16_t refused[] = {0u, 1u, 799u, 5001u, 5260u, 0xFFFFu};
    for (unsigned i = 0u; i < (sizeof(refused) / sizeof(refused[0])); i++) {
        uint16_t code = 0xABCDu;
        TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_dac_nominal_code(refused[i], &code));
        TEST_ASSERT_EQUAL_HEX16(0xABCDu, code);
    }
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_dac_nominal_code(3300u, NULL));
}

static void test_dac_nominal_code_is_the_nearest_code(void)
{
    const uint32_t half_step_uv = (PP_DAC_NOMINAL_STEP_NV / 2000u) + 1u;
    uint16_t previous = 0u;
    for (uint16_t millivolts = PP_SETPOINT_MIN_MV; millivolts <= PP_SETPOINT_MAX_MV; millivolts++) {
        uint16_t code = 0u;
        TEST_ASSERT_EQUAL(PP_OK, pp_dac_nominal_code(millivolts, &code));
        TEST_ASSERT_GREATER_OR_EQUAL_UINT16(previous, code);
        TEST_ASSERT_LESS_OR_EQUAL_UINT16(PP_DAC_CODE_MAX, code);
        previous = code;

        uint32_t wanted = (uint32_t)millivolts * 1000u;
        uint32_t got = pp_dac_nominal_microvolts(code);
        uint32_t error = (got > wanted) ? (got - wanted) : (wanted - got);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(half_step_uv, error);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dac_frame_is_the_command_nibble_and_the_code);
    RUN_TEST(test_dac_frame_never_sends_another_command_nibble);
    RUN_TEST(test_dac_frame_limits_a_code_that_is_too_large);
    RUN_TEST(test_dac_nominal_output_of_a_code);
    RUN_TEST(test_dac_nominal_codes_of_the_two_limits);
    RUN_TEST(test_dac_nominal_code_refuses_a_set_point_outside_the_limits);
    RUN_TEST(test_dac_nominal_code_is_the_nearest_code);
    return UNITY_END();
}
