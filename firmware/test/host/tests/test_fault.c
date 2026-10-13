/*
 * The fault causes against section 6.4 of the specification and the fault
 * flags of section 7.4.
 */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "app/pp_fault.h"
#include "proto/pp_proto_defs.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* Section 6.4 with rule F-19 and F-8: fifteen causes. */
static void test_cause_every_cause_has_its_own_bit(void)
{
    TEST_ASSERT_EQUAL_INT(15, PP_CAUSE_COUNT);
    uint32_t seen = 0u;
    for (int cause = 0; cause < PP_CAUSE_COUNT; cause++) {
        uint32_t bit = pp_cause_bit((pp_cause_t)cause);
        TEST_ASSERT_NOT_EQUAL_UINT32(0u, bit);
        TEST_ASSERT_EQUAL_HEX32(0u, bit & (bit - 1u)); /* one bit only */
        TEST_ASSERT_EQUAL_HEX32(0u, seen & bit);
        seen |= bit;
    }
    TEST_ASSERT_EQUAL_HEX32(0x7FFFu, seen);
    TEST_ASSERT_EQUAL_HEX32(seen, pp_cause_all());
}

static void test_cause_a_value_that_is_no_cause_has_no_bit(void)
{
    TEST_ASSERT_EQUAL_HEX32(0u, pp_cause_bit(PP_CAUSE_COUNT));
    TEST_ASSERT_EQUAL_HEX32(0u, pp_cause_bit((pp_cause_t)31));
    TEST_ASSERT_EQUAL_HEX32(0u, pp_cause_bit((pp_cause_t)-1));
    TEST_ASSERT_FALSE(pp_cause_is_report_only(PP_CAUSE_COUNT));
    TEST_ASSERT_EQUAL_STRING("unknown", pp_cause_name(PP_CAUSE_COUNT));
    TEST_ASSERT_EQUAL_STRING("unknown", pp_cause_name((pp_cause_t)-1));
}

/* F-8: a watchdog restart sets a flag and leaves the instrument in IDLE. */
static void test_cause_only_the_watchdog_restart_is_a_report(void)
{
    for (int cause = 0; cause < PP_CAUSE_COUNT; cause++) {
        TEST_ASSERT_EQUAL(cause == PP_CAUSE_WATCHDOG, pp_cause_is_report_only((pp_cause_t)cause));
    }
}

static void test_cause_which_sets_need_the_fault_state(void)
{
    uint32_t watchdog = pp_cause_bit(PP_CAUSE_WATCHDOG);
    TEST_ASSERT_FALSE(pp_cause_needs_fault_state(0u));
    TEST_ASSERT_FALSE(pp_cause_needs_fault_state(watchdog));
    for (int cause = 0; cause < PP_CAUSE_COUNT; cause++) {
        if (cause != PP_CAUSE_WATCHDOG) {
            uint32_t bit = pp_cause_bit((pp_cause_t)cause);
            TEST_ASSERT_TRUE(pp_cause_needs_fault_state(bit));
            TEST_ASSERT_TRUE(pp_cause_needs_fault_state(bit | watchdog));
        }
    }
    /* Bits that are no cause mean nothing. */
    TEST_ASSERT_FALSE(pp_cause_needs_fault_state(0xFFFF8000u));
}

/* Section 7.4 since version 2 (D-103): one flag for each cause. */
static void test_cause_every_cause_reports_its_own_flag(void)
{
    static const struct {
        pp_cause_t cause;
        uint16_t flag;
    } flags[] = {
        {PP_CAUSE_OVERCURRENT, PP_FAULT_OVERCURRENT},
        {PP_CAUSE_START_OVERCURRENT, PP_FAULT_START_OVERCURRENT},
        {PP_CAUSE_SEQUENCER, PP_FAULT_SEQUENCER},
        {PP_CAUSE_REVERSE_CURRENT, PP_FAULT_REVERSE_CURRENT},
        {PP_CAUSE_CARRIER_SUPPLY, PP_FAULT_CARRIER_SUPPLY},
        {PP_CAUSE_ANALOG_RAILS, PP_FAULT_ANALOG_RAILS},
        {PP_CAUSE_VIN_OVERVOLTAGE, PP_FAULT_VIN_OVERVOLTAGE},
        {PP_CAUSE_VIN_RANGE, PP_FAULT_VIN_RANGE},
        {PP_CAUSE_POWER_LIMIT, PP_FAULT_POWER_LIMIT},
        {PP_CAUSE_THERMAL, PP_FAULT_THERMAL},
        {PP_CAUSE_SOURCE_OVERLOAD, PP_FAULT_SOURCE_OVERLOAD},
        {PP_CAUSE_EXTERNAL_VOLTAGE, PP_FAULT_EXTERNAL_VOLTAGE},
        {PP_CAUSE_START_CHECK, PP_FAULT_START_CHECK},
        {PP_CAUSE_SELFTEST, PP_FAULT_SELFTEST},
        {PP_CAUSE_WATCHDOG, PP_FAULT_WATCHDOG},
    };
    TEST_ASSERT_EQUAL_size_t((size_t)PP_CAUSE_COUNT, sizeof(flags) / sizeof(flags[0]));
    for (unsigned i = 0u; i < (sizeof(flags) / sizeof(flags[0])); i++) {
        TEST_ASSERT_EQUAL_HEX16_MESSAGE(flags[i].flag,
                                        pp_cause_wire_flags(pp_cause_bit(flags[i].cause)),
                                        pp_cause_name(flags[i].cause));
    }
}

static void test_cause_no_two_causes_share_a_flag(void)
{
    uint16_t seen = 0u;
    for (int cause = 0; cause < PP_CAUSE_COUNT; cause++) {
        uint16_t flags = pp_cause_wire_flags(pp_cause_bit((pp_cause_t)cause));
        TEST_ASSERT_NOT_EQUAL_HEX16(0u, flags);
        TEST_ASSERT_EQUAL_HEX16(0u, flags & (uint16_t)(flags - 1u)); /* one bit only */
        TEST_ASSERT_EQUAL_HEX16(0u, seen & flags);
        seen = (uint16_t)(seen | flags);
    }
    TEST_ASSERT_EQUAL_HEX16(0x7FFFu, seen);
}

static void test_cause_flags_of_a_set_combine(void)
{
    uint32_t set = pp_cause_bit(PP_CAUSE_THERMAL) | pp_cause_bit(PP_CAUSE_ANALOG_RAILS) |
                   pp_cause_bit(PP_CAUSE_WATCHDOG);
    TEST_ASSERT_EQUAL_HEX16(PP_FAULT_THERMAL | PP_FAULT_ANALOG_RAILS | PP_FAULT_WATCHDOG,
                            pp_cause_wire_flags(set));
    TEST_ASSERT_EQUAL_HEX16(0u, pp_cause_wire_flags(0u));
    TEST_ASSERT_EQUAL_HEX16(0u, pp_cause_wire_flags(0xFFFF8000u));
    TEST_ASSERT_EQUAL_HEX16(0x7FFFu, pp_cause_wire_flags(pp_cause_all()));
}

static void test_cause_names_are_distinct(void)
{
    for (int a = 0; a < PP_CAUSE_COUNT; a++) {
        const char *name = pp_cause_name((pp_cause_t)a);
        TEST_ASSERT_NOT_NULL(name);
        TEST_ASSERT_NOT_EQUAL_INT(0, (int)strlen(name));
        for (int b = a + 1; b < PP_CAUSE_COUNT; b++) {
            TEST_ASSERT_NOT_EQUAL_INT(0, strcmp(name, pp_cause_name((pp_cause_t)b)));
        }
    }
    TEST_ASSERT_EQUAL_STRING("over-current", pp_cause_name(PP_CAUSE_OVERCURRENT));
    TEST_ASSERT_EQUAL_STRING("watchdog restart", pp_cause_name(PP_CAUSE_WATCHDOG));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cause_every_cause_has_its_own_bit);
    RUN_TEST(test_cause_a_value_that_is_no_cause_has_no_bit);
    RUN_TEST(test_cause_only_the_watchdog_restart_is_a_report);
    RUN_TEST(test_cause_which_sets_need_the_fault_state);
    RUN_TEST(test_cause_every_cause_reports_its_own_flag);
    RUN_TEST(test_cause_no_two_causes_share_a_flag);
    RUN_TEST(test_cause_flags_of_a_set_combine);
    RUN_TEST(test_cause_names_are_distinct);
    return UNITY_END();
}
