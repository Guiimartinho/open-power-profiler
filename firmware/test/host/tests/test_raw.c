#include "unity.h"

#include <stdint.h>

#include "acq/pp_raw.h"
#include "proto/pp_sample.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/*
 * What the capture program does: sixteen clocks, most significant bit first,
 * each shifting the word left by two and taking the register pin as the upper
 * bit of the pair and the converter pin as the lower one.
 */
static uint32_t shift_in(uint16_t adc, uint16_t side)
{
    uint32_t word = 0u;
    for (int bit = 15; bit >= 0; bit--) {
        uint32_t adc_pin = ((uint32_t)adc >> bit) & 1u;
        uint32_t side_pin = ((uint32_t)side >> bit) & 1u;
        word = (word << 2) | (side_pin << 1) | adc_pin;
    }
    return word;
}

/* A fixed sequence of 16-bit values that covers the bit positions unevenly. */
static uint16_t next_value(uint32_t *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return (uint16_t)(*state >> 16);
}

static void test_raw_first_pair_lands_in_the_top_bits(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x40000000u, shift_in(0x8000u, 0x0000u));
    TEST_ASSERT_EQUAL_HEX32(0x80000000u, shift_in(0x0000u, 0x8000u));
    TEST_ASSERT_EQUAL_HEX32(0x00000001u, shift_in(0x0001u, 0x0000u));
    TEST_ASSERT_EQUAL_HEX32(0x00000002u, shift_in(0x0000u, 0x0001u));
}

static void test_raw_split_recovers_every_single_bit(void)
{
    for (unsigned bit = 0u; bit < 16u; bit++) {
        uint16_t one = (uint16_t)(1u << bit);
        TEST_ASSERT_EQUAL_HEX16(one, pp_raw_adc(shift_in(one, 0u)));
        TEST_ASSERT_EQUAL_HEX16(0u, pp_raw_side(shift_in(one, 0u)));
        TEST_ASSERT_EQUAL_HEX16(one, pp_raw_side(shift_in(0u, one)));
        TEST_ASSERT_EQUAL_HEX16(0u, pp_raw_adc(shift_in(0u, one)));
    }
}

static void test_raw_split_keeps_the_two_streams_apart(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, pp_raw_adc(shift_in(0xFFFFu, 0x0000u)));
    TEST_ASSERT_EQUAL_HEX16(0x0000u, pp_raw_side(shift_in(0xFFFFu, 0x0000u)));
    TEST_ASSERT_EQUAL_HEX16(0x0000u, pp_raw_adc(shift_in(0x0000u, 0xFFFFu)));
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, pp_raw_side(shift_in(0x0000u, 0xFFFFu)));
}

static void test_raw_split_recovers_mixed_values(void)
{
    uint32_t state = 0x1234ABCDu;
    for (unsigned i = 0u; i < 2000u; i++) {
        uint16_t adc = next_value(&state);
        uint16_t side = next_value(&state);
        uint32_t raw = shift_in(adc, side);
        TEST_ASSERT_EQUAL_HEX16(adc, pp_raw_adc(raw));
        TEST_ASSERT_EQUAL_HEX16(side, pp_raw_side(raw));
    }
}

static void test_raw_pack_builds_the_word_of_the_capture_program(void)
{
    uint32_t state = 0x00C0FFEEu;
    for (unsigned i = 0u; i < 2000u; i++) {
        uint16_t adc = next_value(&state);
        uint16_t side = next_value(&state);
        TEST_ASSERT_EQUAL_HEX32(shift_in(adc, side), pp_raw_pack(adc, side));
    }
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, pp_raw_pack(0xFFFFu, 0xFFFFu));
    TEST_ASSERT_EQUAL_HEX32(0x00000000u, pp_raw_pack(0x0000u, 0x0000u));
}

static void test_side_range_is_the_first_two_bits_shifted(void)
{
    /* MUX_A1 is shifted first and MUX_A0 second (section 4.7). */
    TEST_ASSERT_EQUAL_UINT8(0u, pp_side_range(0x0000u));
    TEST_ASSERT_EQUAL_UINT8(1u, pp_side_range(0x4000u));
    TEST_ASSERT_EQUAL_UINT8(2u, pp_side_range(0x8000u));
    TEST_ASSERT_EQUAL_UINT8(3u, pp_side_range(0xC000u));
    /* No other bit changes the range. */
    TEST_ASSERT_EQUAL_UINT8(0u, pp_side_range(0x3FFFu));
}

static void test_side_logic_is_the_last_eight_bits_shifted(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00u, pp_side_logic(0xFF00u));
    TEST_ASSERT_EQUAL_HEX8(0xA5u, pp_side_logic(0x00A5u));
    /* D7 is shifted before D0. */
    TEST_ASSERT_EQUAL_HEX8(0x80u, pp_side_logic(0x0080u));
    TEST_ASSERT_EQUAL_HEX8(0x01u, pp_side_logic(0x0001u));
}

static void test_side_flags_follow_the_order_of_the_registers(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x2000u, PP_SIDE_OUTPUT_BIT);
    TEST_ASSERT_EQUAL_HEX16(0x1000u, PP_SIDE_VIN_OV_BIT);
    TEST_ASSERT_EQUAL_HEX16(0x0800u, PP_SIDE_CMP_OC_BIT);
    TEST_ASSERT_EQUAL_HEX16(0x0400u, PP_SIDE_CMP_JUMP_BIT);
    TEST_ASSERT_EQUAL_HEX16(0x0200u, PP_SIDE_CMP_UP_BIT);
    TEST_ASSERT_EQUAL_HEX16(0x0100u, PP_SIDE_PWR_GOOD_BIT);

    TEST_ASSERT_TRUE(pp_side_output_on(PP_SIDE_OUTPUT_BIT));
    TEST_ASSERT_FALSE(pp_side_output_on((uint16_t)~PP_SIDE_OUTPUT_BIT));
    TEST_ASSERT_TRUE(pp_side_cmp_jump(PP_SIDE_CMP_JUMP_BIT));
    TEST_ASSERT_FALSE(pp_side_cmp_jump((uint16_t)~PP_SIDE_CMP_JUMP_BIT));
}

static void test_raw_sample_word_carries_code_range_and_logic(void)
{
    uint16_t side = (uint16_t)(0x8000u | PP_SIDE_CMP_UP_BIT | PP_SIDE_PWR_GOOD_BIT | 0x003Cu);
    uint32_t word = pp_raw_sample_word(0xBEEFu, side);

    TEST_ASSERT_EQUAL_HEX16(0xBEEFu, pp_sample_adc(word));
    TEST_ASSERT_EQUAL_UINT8(2u, pp_sample_range(word));
    TEST_ASSERT_EQUAL_HEX8(0x3Cu, pp_sample_logic(word));
    TEST_ASSERT_FALSE(pp_sample_is_invalid(word));
    TEST_ASSERT_FALSE(pp_sample_has_fault(word));
    TEST_ASSERT_FALSE(pp_sample_is_over_range(word));
    TEST_ASSERT_FALSE(pp_sample_is_under_range(word));
    TEST_ASSERT_FALSE(pp_sample_output_on(word));
}

/* F-35: over-range when the code is 65535 or the jump comparator is set. */
static void test_raw_over_range_by_code_and_by_the_jump_comparator(void)
{
    TEST_ASSERT_FALSE(pp_raw_is_over_range(0xFFFEu, 0u));
    TEST_ASSERT_TRUE(pp_raw_is_over_range(0xFFFFu, 0u));
    TEST_ASSERT_TRUE(pp_raw_is_over_range(0x1234u, PP_SIDE_CMP_JUMP_BIT));
    /* The other comparators alone do not mark a sample. */
    TEST_ASSERT_FALSE(
        pp_raw_is_over_range(0x1234u, (uint16_t)(PP_SIDE_CMP_UP_BIT | PP_SIDE_CMP_OC_BIT)));

    TEST_ASSERT_TRUE(pp_sample_is_over_range(pp_raw_sample_word(0xFFFFu, 0u)));
    TEST_ASSERT_TRUE(pp_sample_is_over_range(pp_raw_sample_word(100u, PP_SIDE_CMP_JUMP_BIT)));
    TEST_ASSERT_FALSE(pp_sample_is_over_range(pp_raw_sample_word(100u, PP_SIDE_CMP_UP_BIT)));
}

/* Section 7.3: bit 22 is the GATE_OUT bit of the sample. */
static void test_raw_sample_word_carries_the_output_request(void)
{
    TEST_ASSERT_TRUE(pp_sample_output_on(pp_raw_sample_word(100u, PP_SIDE_OUTPUT_BIT)));
    TEST_ASSERT_FALSE(pp_sample_output_on(pp_raw_sample_word(100u, (uint16_t)~PP_SIDE_OUTPUT_BIT)));
}

static void test_raw_sample_word_leaves_the_other_marks_clear(void)
{
    uint32_t word = pp_raw_sample_word(0xFFFFu, 0xFFFFu);
    uint32_t fields = ((uint32_t)PP_SAMPLE_ADC_MASK << PP_SAMPLE_ADC_SHIFT) |
                      ((uint32_t)PP_SAMPLE_RANGE_MASK << PP_SAMPLE_RANGE_SHIFT) |
                      PP_SAMPLE_OVER_RANGE_BIT | PP_SAMPLE_OUTPUT_BIT |
                      ((uint32_t)PP_SAMPLE_LOGIC_MASK << PP_SAMPLE_LOGIC_SHIFT);
    TEST_ASSERT_EQUAL_HEX32(fields, word);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_raw_first_pair_lands_in_the_top_bits);
    RUN_TEST(test_raw_split_recovers_every_single_bit);
    RUN_TEST(test_raw_split_keeps_the_two_streams_apart);
    RUN_TEST(test_raw_split_recovers_mixed_values);
    RUN_TEST(test_raw_pack_builds_the_word_of_the_capture_program);
    RUN_TEST(test_side_range_is_the_first_two_bits_shifted);
    RUN_TEST(test_side_logic_is_the_last_eight_bits_shifted);
    RUN_TEST(test_side_flags_follow_the_order_of_the_registers);
    RUN_TEST(test_raw_sample_word_carries_code_range_and_logic);
    RUN_TEST(test_raw_over_range_by_code_and_by_the_jump_comparator);
    RUN_TEST(test_raw_sample_word_carries_the_output_request);
    RUN_TEST(test_raw_sample_word_leaves_the_other_marks_clear);
    return UNITY_END();
}
