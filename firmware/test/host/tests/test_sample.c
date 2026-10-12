#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pp_proto_vectors.h"
#include "proto/pp_sample.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_sample_pack_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_SAMPLE_VECTOR_COUNT; i++) {
        const pp_sample_vector_t *vector = &pp_sample_vectors[i];
        pp_sample_t sample = {
            .adc = vector->adc,
            .range = vector->range,
            .invalid = (vector->invalid != 0u),
            .fault = (vector->fault != 0u),
            .over_range = (vector->over_range != 0u),
            .under_range = (vector->under_range != 0u),
            .output = (vector->output != 0u),
            .logic = vector->logic,
        };

        TEST_ASSERT_EQUAL_HEX32_MESSAGE(vector->word, pp_sample_pack(&sample), vector->name);
    }
}

static void test_sample_unpack_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_SAMPLE_VECTOR_COUNT; i++) {
        const pp_sample_vector_t *vector = &pp_sample_vectors[i];
        pp_sample_t sample;

        pp_sample_unpack(vector->word, &sample);

        TEST_ASSERT_EQUAL_HEX16_MESSAGE(vector->adc, sample.adc, vector->name);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(vector->range, sample.range, vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->invalid != 0u, sample.invalid, vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->fault != 0u, sample.fault, vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->over_range != 0u, sample.over_range, vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->under_range != 0u, sample.under_range, vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->output != 0u, sample.output, vector->name);
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(vector->logic, sample.logic, vector->name);
    }
}

static void test_sample_field_readers_match_unpack(void)
{
    for (size_t i = 0u; i < PP_SAMPLE_VECTOR_COUNT; i++) {
        const pp_sample_vector_t *vector = &pp_sample_vectors[i];
        uint32_t word = vector->word;

        TEST_ASSERT_EQUAL_HEX16_MESSAGE(vector->adc, pp_sample_adc(word), vector->name);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(vector->range, pp_sample_range(word), vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->invalid != 0u, pp_sample_is_invalid(word), vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->fault != 0u, pp_sample_has_fault(word), vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->over_range != 0u, pp_sample_is_over_range(word),
                                  vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->under_range != 0u, pp_sample_is_under_range(word),
                                  vector->name);
        TEST_ASSERT_EQUAL_MESSAGE(vector->output != 0u, pp_sample_output_on(word), vector->name);
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(vector->logic, pp_sample_logic(word), vector->name);
    }
}

static void test_sample_fields_do_not_overlap(void)
{
    const pp_sample_t fields[] = {
        {.adc = 0xFFFFu},     {.range = 3u},         {.invalid = true}, {.fault = true},
        {.over_range = true}, {.under_range = true}, {.output = true},  {.logic = 0xFFu},
    };
    uint32_t seen = 0u;
    for (size_t i = 0u; i < (sizeof(fields) / sizeof(fields[0])); i++) {
        uint32_t word = pp_sample_pack(&fields[i]);
        TEST_ASSERT_NOT_EQUAL_UINT32(0u, word);
        TEST_ASSERT_EQUAL_HEX32(0u, seen & word);
        seen |= word;
    }
    /* Everything except the one reserved bit is in use. */
    TEST_ASSERT_EQUAL_HEX32(0xFF7FFFFFu, seen);
}

/* Section 7.3: bits 18 to 22 are the marks, bit 23 is reserved. */
static void test_sample_marks_sit_on_the_bits_of_the_specification(void)
{
    TEST_ASSERT_EQUAL_HEX32(1u << 18, PP_SAMPLE_INVALID_BIT);
    TEST_ASSERT_EQUAL_HEX32(1u << 19, PP_SAMPLE_FAULT_BIT);
    TEST_ASSERT_EQUAL_HEX32(1u << 20, PP_SAMPLE_OVER_RANGE_BIT);
    TEST_ASSERT_EQUAL_HEX32(1u << 21, PP_SAMPLE_UNDER_RANGE_BIT);
    TEST_ASSERT_EQUAL_HEX32(1u << 22, PP_SAMPLE_OUTPUT_BIT);
    TEST_ASSERT_EQUAL_HEX32(1u << 23,
                            (uint32_t)PP_SAMPLE_RESERVED_MASK << PP_SAMPLE_RESERVED_SHIFT);
}

/* Section 9: settling window, over-range and under-range are the flagged samples. */
static void test_sample_is_flagged_for_the_three_marks_of_a_measurement(void)
{
    TEST_ASSERT_FALSE(pp_sample_is_flagged(0u));
    TEST_ASSERT_TRUE(pp_sample_is_flagged(PP_SAMPLE_INVALID_BIT));
    TEST_ASSERT_TRUE(pp_sample_is_flagged(PP_SAMPLE_OVER_RANGE_BIT));
    TEST_ASSERT_TRUE(pp_sample_is_flagged(PP_SAMPLE_UNDER_RANGE_BIT));
    /* The fault latch and the output request are context, not marks. */
    TEST_ASSERT_FALSE(pp_sample_is_flagged(PP_SAMPLE_FAULT_BIT));
    TEST_ASSERT_FALSE(pp_sample_is_flagged(PP_SAMPLE_OUTPUT_BIT));
    TEST_ASSERT_FALSE(pp_sample_is_flagged(0xFF03FFFFu | PP_SAMPLE_OUTPUT_BIT));
}

static void test_sample_pack_leaves_the_reserved_bit_clear(void)
{
    pp_sample_t all = {.adc = 0xFFFFu,
                       .range = 3u,
                       .invalid = true,
                       .fault = true,
                       .over_range = true,
                       .under_range = true,
                       .output = true,
                       .logic = 0xFFu};
    uint32_t reserved = (uint32_t)PP_SAMPLE_RESERVED_MASK << PP_SAMPLE_RESERVED_SHIFT;

    TEST_ASSERT_EQUAL_HEX32(0u, pp_sample_pack(&all) & reserved);
}

static void test_sample_pack_truncates_a_range_that_does_not_fit(void)
{
    pp_sample_t sample = {.adc = 0u, .range = 0xFFu};

    uint32_t word = pp_sample_pack(&sample);

    TEST_ASSERT_EQUAL_UINT8(PP_SAMPLE_RANGE_MASK, pp_sample_range(word));
    TEST_ASSERT_FALSE(pp_sample_is_invalid(word));
    TEST_ASSERT_FALSE(pp_sample_has_fault(word));
}

static void test_sample_unpack_ignores_the_reserved_bit(void)
{
    pp_sample_t clean;
    pp_sample_t dirty;
    uint32_t word = 0xA5021234u;
    uint32_t reserved = (uint32_t)PP_SAMPLE_RESERVED_MASK << PP_SAMPLE_RESERVED_SHIFT;

    pp_sample_unpack(word, &clean);
    pp_sample_unpack(word | reserved, &dirty);

    TEST_ASSERT_EQUAL_HEX16(clean.adc, dirty.adc);
    TEST_ASSERT_EQUAL_UINT8(clean.range, dirty.range);
    TEST_ASSERT_EQUAL(clean.invalid, dirty.invalid);
    TEST_ASSERT_EQUAL(clean.fault, dirty.fault);
    TEST_ASSERT_EQUAL(clean.over_range, dirty.over_range);
    TEST_ASSERT_EQUAL(clean.under_range, dirty.under_range);
    TEST_ASSERT_EQUAL(clean.output, dirty.output);
    TEST_ASSERT_EQUAL_HEX8(clean.logic, dirty.logic);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sample_pack_matches_the_shared_vectors);
    RUN_TEST(test_sample_unpack_matches_the_shared_vectors);
    RUN_TEST(test_sample_field_readers_match_unpack);
    RUN_TEST(test_sample_fields_do_not_overlap);
    RUN_TEST(test_sample_marks_sit_on_the_bits_of_the_specification);
    RUN_TEST(test_sample_is_flagged_for_the_three_marks_of_a_measurement);
    RUN_TEST(test_sample_pack_leaves_the_reserved_bit_clear);
    RUN_TEST(test_sample_pack_truncates_a_range_that_does_not_fit);
    RUN_TEST(test_sample_unpack_ignores_the_reserved_bit);
    return UNITY_END();
}
