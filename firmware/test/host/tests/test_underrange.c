/*
 * The under-range rule against rule F-22 of section 6.6 of the
 * specification. The times of the rule are counts of samples; the tests use
 * small counts and check the numbers of F-22 once.
 */
#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "acq/pp_underrange.h"

#define THRESHOLD 650u
#define QUALIFY 5u
#define HOLD 20u
#define FAULT 12u

#define LOW 100u   /* a code below the threshold */
#define HIGH 1313u /* the pedestal: no current */

static pp_underrange_t rule;

void setUp(void)
{
    const pp_underrange_config_t config = {
        .threshold = THRESHOLD, .qualify = QUALIFY, .hold = HOLD, .fault = FAULT};
    TEST_ASSERT_EQUAL(PP_OK, pp_underrange_init(&rule, &config));
}

void tearDown(void)
{
}

/* Feed samples with one code and range; return the result of the last one. */
static unsigned feed(uint16_t adc, uint8_t range, unsigned count)
{
    unsigned result = 0u;
    for (unsigned i = 0u; i < count; i++) {
        result = pp_underrange_update(&rule, adc, range, true);
    }
    return result;
}

static void test_underrange_init_checks_its_arguments(void)
{
    pp_underrange_config_t config = {.threshold = 650u, .qualify = 1u, .hold = 0u, .fault = 1u};
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_underrange_init(NULL, &config));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_underrange_init(&rule, NULL));
    config.qualify = 0u;
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_underrange_init(&rule, &config));
    config.qualify = 1u;
    config.fault = 0u;
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_underrange_init(&rule, &config));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_underrange_init_default(NULL));
}

/* F-22: below 650, 100 samples, 1 s and 100 ms at 100 kSPS. */
static void test_underrange_defaults_are_the_numbers_of_f22(void)
{
    TEST_ASSERT_EQUAL(PP_OK, pp_underrange_init_default(&rule));
    TEST_ASSERT_EQUAL_UINT16(650u, rule.config.threshold);
    TEST_ASSERT_EQUAL_UINT16(100u, rule.config.qualify);
    TEST_ASSERT_EQUAL_UINT32(100000u, rule.config.hold);
    TEST_ASSERT_EQUAL_UINT32(10000u, rule.config.fault);
}

static void test_underrange_nothing_happens_at_the_pedestal(void)
{
    TEST_ASSERT_EQUAL_UINT(0u, feed(HIGH, 1u, 1000u));
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));
}

static void test_underrange_threshold_is_the_first_code_that_is_not_low(void)
{
    feed((uint16_t)(THRESHOLD - 1u), 1u, QUALIFY);
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));
    pp_underrange_reset(&rule);
    TEST_ASSERT_EQUAL_UINT(0u, feed(THRESHOLD, 1u, 100u));
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));
}

/* F-22: the flag rises after the samples in a row, not before. */
static void test_underrange_flag_rises_after_the_qualify_count(void)
{
    TEST_ASSERT_EQUAL_UINT(0u, feed(LOW, 1u, QUALIFY - 1u));
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));

    unsigned result = feed(LOW, 1u, 1u);
    TEST_ASSERT_BITS_HIGH(PP_UNDER_MARK, result);
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));
}

static void test_underrange_a_sample_above_restarts_the_count(void)
{
    feed(LOW, 1u, QUALIFY - 1u);
    feed(HIGH, 1u, 1u);
    TEST_ASSERT_EQUAL_UINT(0u, feed(LOW, 1u, QUALIFY - 1u));
    TEST_ASSERT_BITS_HIGH(PP_UNDER_MARK, feed(LOW, 1u, 1u));
}

/* F-22: range 3 is requested at once when the flag rises. */
static void test_underrange_requests_range_3_when_the_flag_rises(void)
{
    feed(LOW, 0u, QUALIFY - 1u);
    unsigned result = feed(LOW, 0u, 1u);
    TEST_ASSERT_EQUAL_HEX(PP_UNDER_MARK | PP_UNDER_REQUEST_R3, result);
}

static void test_underrange_repeats_the_request_while_the_range_does_not_follow(void)
{
    unsigned requests = 0u;
    for (unsigned i = 0u; i < (4u * QUALIFY); i++) {
        if ((pp_underrange_update(&rule, LOW, 2u, true) & PP_UNDER_REQUEST_R3) != 0u) {
            requests++;
        }
    }
    /* At the rise, then once per qualify time: samples 5, 10, 15 and 20. */
    TEST_ASSERT_EQUAL_UINT(4u, requests);
}

static void test_underrange_asks_for_nothing_once_range_3_is_reached(void)
{
    feed(LOW, 2u, QUALIFY);
    for (unsigned i = 0u; i < (3u * QUALIFY); i++) {
        unsigned result = pp_underrange_update(&rule, LOW, 3u, true);
        TEST_ASSERT_BITS_LOW(PP_UNDER_REQUEST_R3, result);
        TEST_ASSERT_BITS_HIGH(PP_UNDER_MARK, result);
    }
}

static void test_underrange_no_request_when_the_flag_rises_in_range_3(void)
{
    feed(LOW, 3u, QUALIFY - 1u);
    TEST_ASSERT_EQUAL_HEX(PP_UNDER_MARK, feed(LOW, 3u, 1u));
}

/* F-22: no step down until the hold time after the flag cleared. */
static void test_underrange_blocks_step_down_for_the_hold_time(void)
{
    feed(LOW, 3u, QUALIFY);
    TEST_ASSERT_EQUAL_UINT(0u, feed(HIGH, 3u, 1u)); /* the flag clears */
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));

    feed(HIGH, 3u, HOLD - 1u);
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));
    feed(HIGH, 3u, 1u);
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));
}

static void test_underrange_a_new_flag_starts_the_hold_again(void)
{
    feed(LOW, 3u, QUALIFY);
    feed(HIGH, 3u, HOLD / 2u);
    feed(LOW, 3u, QUALIFY);
    feed(HIGH, 3u, HOLD);
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));
    feed(HIGH, 3u, 1u);
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));
}

/* F-22: a code that stays low in range 3 for the fault time is a fault. */
static void test_underrange_reports_the_fault_once_after_the_fault_time(void)
{
    unsigned faults = 0u;
    unsigned first = 0u;
    for (unsigned i = 1u; i <= (3u * FAULT); i++) {
        if ((pp_underrange_update(&rule, LOW, 3u, true) & PP_UNDER_FAULT) != 0u) {
            faults++;
            first = (first == 0u) ? i : first;
        }
    }
    TEST_ASSERT_EQUAL_UINT(1u, faults);
    TEST_ASSERT_EQUAL_UINT(FAULT, first);
}

static void test_underrange_time_outside_range_3_does_not_count_for_the_fault(void)
{
    TEST_ASSERT_BITS_LOW(PP_UNDER_FAULT, feed(LOW, 2u, 5u * FAULT));
    /* The time counts from the first sample in range 3. */
    TEST_ASSERT_BITS_LOW(PP_UNDER_FAULT, feed(LOW, 3u, FAULT - 1u));
    TEST_ASSERT_BITS_HIGH(PP_UNDER_FAULT, feed(LOW, 3u, 1u));
}

static void test_underrange_a_sample_above_ends_the_fault_count(void)
{
    feed(LOW, 3u, FAULT - 1u);
    feed(HIGH, 3u, 1u);
    TEST_ASSERT_BITS_LOW(PP_UNDER_FAULT, feed(LOW, 3u, FAULT - 1u));
    TEST_ASSERT_BITS_HIGH(PP_UNDER_FAULT, feed(LOW, 3u, 1u));
}

/* F-22: the rule watches while a mode pair is closed. */
static void test_underrange_rests_while_no_path_is_closed(void)
{
    for (unsigned i = 0u; i < (5u * FAULT); i++) {
        TEST_ASSERT_EQUAL_UINT(0u, pp_underrange_update(&rule, LOW, 3u, false));
    }
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));

    /* An open path also ends a flag and its hold. */
    feed(LOW, 1u, QUALIFY);
    TEST_ASSERT_TRUE(pp_underrange_blocks_step_down(&rule));
    TEST_ASSERT_EQUAL_UINT(0u, pp_underrange_update(&rule, LOW, 1u, false));
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(&rule));
}

static void test_underrange_functions_tolerate_null(void)
{
    TEST_ASSERT_EQUAL_UINT(0u, pp_underrange_update(NULL, LOW, 3u, true));
    TEST_ASSERT_FALSE(pp_underrange_blocks_step_down(NULL));
    pp_underrange_reset(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_underrange_init_checks_its_arguments);
    RUN_TEST(test_underrange_defaults_are_the_numbers_of_f22);
    RUN_TEST(test_underrange_nothing_happens_at_the_pedestal);
    RUN_TEST(test_underrange_threshold_is_the_first_code_that_is_not_low);
    RUN_TEST(test_underrange_flag_rises_after_the_qualify_count);
    RUN_TEST(test_underrange_a_sample_above_restarts_the_count);
    RUN_TEST(test_underrange_requests_range_3_when_the_flag_rises);
    RUN_TEST(test_underrange_repeats_the_request_while_the_range_does_not_follow);
    RUN_TEST(test_underrange_asks_for_nothing_once_range_3_is_reached);
    RUN_TEST(test_underrange_no_request_when_the_flag_rises_in_range_3);
    RUN_TEST(test_underrange_blocks_step_down_for_the_hold_time);
    RUN_TEST(test_underrange_a_new_flag_starts_the_hold_again);
    RUN_TEST(test_underrange_reports_the_fault_once_after_the_fault_time);
    RUN_TEST(test_underrange_time_outside_range_3_does_not_count_for_the_fault);
    RUN_TEST(test_underrange_a_sample_above_ends_the_fault_count);
    RUN_TEST(test_underrange_rests_while_no_path_is_closed);
    RUN_TEST(test_underrange_functions_tolerate_null);
    return UNITY_END();
}
