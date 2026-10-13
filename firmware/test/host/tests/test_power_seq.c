/*
 * The sequences of DUT power on and off, read back against rules F-23 to
 * F-31 of section 6.6 of the specification. The operations are a recorder:
 * every action is written down with its time, and the checks answer what the
 * test tells them to.
 */
#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app/pp_fault.h"
#include "app/pp_power_seq.h"
#include "app/pp_sequence.h"

#define LOG_SIZE 32u
#define POLL_US 250u     /* period of the polls */
#define END_US 6000000u  /* no sequence takes longer */
#define NEVER UINT64_MAX /* time of an action that was not done */
#define MS(value) ((uint64_t)(value) * 1000u)

typedef struct {
    uint64_t now;
    struct {
        uint64_t at;
        uint8_t action;
    } log[LOG_SIZE];
    size_t count;
    bool holds[PP_CHK_COUNT];
    uint64_t holds_from[PP_CHK_COUNT]; /* a check that begins to hold at a time */
    uint64_t first_asked[PP_CHK_COUNT];
    uint64_t ended_at;
} recorder_t;

static recorder_t rec;
static pp_seq_t seq;

static void rec_act(void *ctx, uint8_t action)
{
    recorder_t *r = (recorder_t *)ctx;
    TEST_ASSERT_LESS_THAN_size_t(LOG_SIZE, r->count);
    r->log[r->count].at = r->now;
    r->log[r->count].action = action;
    r->count++;
}

static bool rec_check(void *ctx, uint8_t check)
{
    recorder_t *r = (recorder_t *)ctx;
    TEST_ASSERT_LESS_THAN_UINT8(PP_CHK_COUNT, check);
    if (r->first_asked[check] == NEVER) {
        r->first_asked[check] = r->now;
    }
    return r->holds[check] || (r->now >= r->holds_from[check]);
}

/* Run a sequence from time zero until it ends, polling as the loop would. */
static pp_seq_state_t run(pp_power_seq_id_t id)
{
    size_t count = 0u;
    const pp_step_t *steps = pp_power_seq(id, &count);
    TEST_ASSERT_NOT_NULL(steps);
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, count));
    pp_seq_state_t state = PP_SEQ_RUNNING;
    for (rec.now = 0u; (state == PP_SEQ_RUNNING) && (rec.now <= END_US); rec.now += POLL_US) {
        state = pp_seq_poll(&seq, rec.now);
        rec.ended_at = rec.now;
    }
    return state;
}

/* Time of the first time an action was done, or NEVER. */
static uint64_t when(pp_power_action_t action)
{
    for (size_t i = 0u; i < rec.count; i++) {
        if (rec.log[i].action == (uint8_t)action) {
            return rec.log[i].at;
        }
    }
    return NEVER;
}

static void assert_actions(const pp_power_action_t *expected, size_t count)
{
    TEST_ASSERT_EQUAL_size_t(count, rec.count);
    for (size_t i = 0u; i < count; i++) {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)expected[i], rec.log[i].action);
    }
}

/* The wait between two actions is the time of the rule, within one poll. */
static void assert_wait(pp_power_action_t first, pp_power_action_t second, uint64_t rule_us)
{
    uint64_t gap = when(second) - when(first);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(rule_us, gap);
    TEST_ASSERT_LESS_OR_EQUAL_UINT64(rule_us + POLL_US, gap);
}

/* How often an action appears in the table of a sequence. */
static size_t in_table(pp_power_seq_id_t id, pp_power_action_t action)
{
    size_t count = 0u;
    const pp_step_t *steps = pp_power_seq(id, &count);
    size_t found = 0u;
    for (size_t i = 0u; i < count; i++) {
        if (steps[i].action == (uint8_t)action) {
            found++;
        }
    }
    return found;
}

void setUp(void)
{
    memset(&rec, 0, sizeof(rec));
    for (size_t i = 0u; i < PP_CHK_COUNT; i++) {
        rec.holds_from[i] = NEVER;
        rec.first_asked[i] = NEVER;
    }
    /* What holds on a healthy instrument with a DUT that draws no current back. */
    rec.holds[PP_CHK_SOURCE_READY] = true;
    rec.holds[PP_CHK_VOUT_AT_SETPOINT] = true;
    rec.holds[PP_CHK_VIN_VALID] = true;
    rec.holds[PP_CHK_LADDER_BELOW_VIN] = true;
    rec.holds[PP_CHK_DUT_ABOVE_SETPOINT] = false;

    const pp_seq_ops_t ops = {.ctx = &rec, .act = rec_act, .check = rec_check};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_init(&seq, &ops));
}

void tearDown(void)
{
}

/* F-28: the eight steps, in order. */
static void test_source_on_follows_the_order_of_f28(void)
{
    static const pp_power_action_t order[] = {
        PP_ACT_DAC_START,      PP_ACT_SMU_ON,    PP_ACT_DAC_WORKING,     PP_ACT_RANGE3,
        PP_ACT_SOURCE_PAIR_ON, PP_ACT_OUTPUT_ON, PP_ACT_RANGING_RELEASE,
    };
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_SOURCE_ON));
    assert_actions(order, sizeof(order) / sizeof(order[0]));
}

/* F-28: 200 ms, 5 ms, 80 ms and 60 ms. F-24: on 50 ms after GATE_OUT. */
static void test_source_on_waits_the_times_of_f28(void)
{
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_SOURCE_ON));
    assert_wait(PP_ACT_DAC_START, PP_ACT_SMU_ON, MS(200u));
    assert_wait(PP_ACT_SMU_ON, PP_ACT_DAC_WORKING, MS(5u));
    assert_wait(PP_ACT_DAC_WORKING, PP_ACT_SOURCE_PAIR_ON, MS(80u));
    assert_wait(PP_ACT_SOURCE_PAIR_ON, PP_ACT_OUTPUT_ON, MS(60u));
    assert_wait(PP_ACT_OUTPUT_ON, PP_ACT_RANGING_RELEASE, MS(50u));
    /* Range 3 is selected before the source pair closes (F-20). */
    TEST_ASSERT_LESS_OR_EQUAL_UINT64(when(PP_ACT_SOURCE_PAIR_ON), when(PP_ACT_RANGE3));
}

/* F-28: the sequence starts only from its conditions. */
static void test_source_on_does_nothing_when_its_conditions_do_not_hold(void)
{
    rec.holds[PP_CHK_SOURCE_READY] = false;
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, run(PP_POWER_SEQ_SOURCE_ON));
    TEST_ASSERT_EQUAL_UINT8(PP_CAUSE_START_CHECK, pp_seq_fault(&seq));
    TEST_ASSERT_EQUAL_size_t(0u, rec.count);
}

/* F-28 (7): channel 0 within 100 mV of the set-point, or a fault. */
static void test_source_on_fails_when_the_output_is_not_at_the_set_point(void)
{
    rec.holds[PP_CHK_VOUT_AT_SETPOINT] = false;
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, run(PP_POWER_SEQ_SOURCE_ON));
    TEST_ASSERT_EQUAL_UINT8(PP_CAUSE_START_CHECK, pp_seq_fault(&seq));
    TEST_ASSERT_EQUAL_UINT64(NEVER, when(PP_ACT_OUTPUT_ON));
    /* The check is made 60 ms after the source pair, not earlier. */
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(when(PP_ACT_SOURCE_PAIR_ON) + MS(60u),
                                        rec.first_asked[PP_CHK_VOUT_AT_SETPOINT]);
}

/* F-28: no other path sets SMU_ON. */
static void test_smu_on_is_set_in_one_place_only(void)
{
    TEST_ASSERT_EQUAL_size_t(1u, in_table(PP_POWER_SEQ_SOURCE_ON, PP_ACT_SMU_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_SOURCE_OFF, PP_ACT_SMU_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_AMPERE_ON, PP_ACT_SMU_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_AMPERE_OFF, PP_ACT_SMU_ON));
}

/* F-23: the two mode requests are never raised by one sequence. */
static void test_a_sequence_raises_one_mode_request_only(void)
{
    TEST_ASSERT_EQUAL_size_t(1u, in_table(PP_POWER_SEQ_SOURCE_ON, PP_ACT_SOURCE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_SOURCE_ON, PP_ACT_AMPERE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(1u, in_table(PP_POWER_SEQ_AMPERE_ON, PP_ACT_AMPERE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_AMPERE_ON, PP_ACT_SOURCE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_SOURCE_OFF, PP_ACT_SOURCE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_SOURCE_OFF, PP_ACT_AMPERE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_AMPERE_OFF, PP_ACT_SOURCE_PAIR_ON));
    TEST_ASSERT_EQUAL_size_t(0u, in_table(PP_POWER_SEQ_AMPERE_OFF, PP_ACT_AMPERE_PAIR_ON));
}

/* F-24, F-25, F-26: range 3, the ampere pair, 40 ms, the output, 50 ms. */
static void test_ampere_on_follows_the_order_and_times_of_f24(void)
{
    static const pp_power_action_t order[] = {
        PP_ACT_RANGE3,
        PP_ACT_AMPERE_PAIR_ON,
        PP_ACT_OUTPUT_ON,
        PP_ACT_RANGING_RELEASE,
    };
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_AMPERE_ON));
    assert_actions(order, sizeof(order) / sizeof(order[0]));
    assert_wait(PP_ACT_AMPERE_PAIR_ON, PP_ACT_OUTPUT_ON, MS(40u));
    assert_wait(PP_ACT_OUTPUT_ON, PP_ACT_RANGING_RELEASE, MS(50u));
}

/* F-26: the ampere pair closes only on a valid VIN. */
static void test_ampere_on_does_nothing_without_a_valid_vin(void)
{
    rec.holds[PP_CHK_VIN_VALID] = false;
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, run(PP_POWER_SEQ_AMPERE_ON));
    TEST_ASSERT_EQUAL_UINT8(PP_CAUSE_VIN_RANGE, pp_seq_fault(&seq));
    TEST_ASSERT_EQUAL_size_t(0u, rec.count);
}

/* F-25: GATE_AMP waits until the ladder node is no higher than VIN. */
static void test_ampere_on_waits_for_the_ladder_node(void)
{
    rec.holds[PP_CHK_LADDER_BELOW_VIN] = false;
    rec.holds_from[PP_CHK_LADDER_BELOW_VIN] = MS(1000u);
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_AMPERE_ON));
    TEST_ASSERT_EQUAL_UINT64(0u, when(PP_ACT_RANGE3));
    TEST_ASSERT_EQUAL_UINT64(MS(1000u), when(PP_ACT_AMPERE_PAIR_ON));
}

/* F-25: after 3 s without that a fault is reported. */
static void test_ampere_on_fails_after_3_seconds_with_the_ladder_node_high(void)
{
    rec.holds[PP_CHK_LADDER_BELOW_VIN] = false;
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, run(PP_POWER_SEQ_AMPERE_ON));
    TEST_ASSERT_EQUAL_UINT8(PP_CAUSE_START_CHECK, pp_seq_fault(&seq));
    TEST_ASSERT_EQUAL_UINT64(MS(3000u), rec.ended_at);
    TEST_ASSERT_EQUAL_UINT64(NEVER, when(PP_ACT_AMPERE_PAIR_ON));
    TEST_ASSERT_EQUAL_UINT64(NEVER, when(PP_ACT_OUTPUT_ON));
}

/* F-29: output pair, source pair, SMU_ON, DAC at zero. */
static void test_source_off_follows_the_order_of_f29(void)
{
    static const pp_power_action_t order[] = {
        PP_ACT_OUTPUT_OFF,
        PP_ACT_SOURCE_PAIR_OFF,
        PP_ACT_SMU_OFF,
        PP_ACT_DAC_ZERO,
    };
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_SOURCE_OFF));
    assert_actions(order, sizeof(order) / sizeof(order[0]));
    /* The output opens in the first poll: nothing waits in front of it. */
    TEST_ASSERT_EQUAL_UINT64(0u, when(PP_ACT_OUTPUT_OFF));
    /* F-23: 5 ms after the request fell before the sequence is over. */
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(when(PP_ACT_SOURCE_PAIR_OFF) + MS(5u), rec.ended_at);
}

/* F-31: with the DUT above the set-point the DAC follows first, then 30 ms. */
static void test_source_off_follows_a_dut_above_the_set_point_first(void)
{
    static const pp_power_action_t order[] = {
        PP_ACT_DAC_FOLLOW, PP_ACT_OUTPUT_OFF, PP_ACT_SOURCE_PAIR_OFF,
        PP_ACT_SMU_OFF,    PP_ACT_DAC_ZERO,
    };
    rec.holds[PP_CHK_DUT_ABOVE_SETPOINT] = true;
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_SOURCE_OFF));
    assert_actions(order, sizeof(order) / sizeof(order[0]));
    assert_wait(PP_ACT_DAC_FOLLOW, PP_ACT_OUTPUT_OFF, MS(30u));
}

/* F-23: the output pair opens first, then the mode pair, then 5 ms. */
static void test_ampere_off_opens_the_output_first(void)
{
    static const pp_power_action_t order[] = {PP_ACT_OUTPUT_OFF, PP_ACT_AMPERE_PAIR_OFF};
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(PP_POWER_SEQ_AMPERE_OFF));
    assert_actions(order, sizeof(order) / sizeof(order[0]));
    TEST_ASSERT_EQUAL_UINT64(0u, when(PP_ACT_OUTPUT_OFF));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(when(PP_ACT_AMPERE_PAIR_OFF) + MS(5u), rec.ended_at);
}

/* F-23: in every sequence the output closes last and opens first. */
static void test_the_output_pair_closes_last_and_opens_first(void)
{
    static const pp_power_seq_id_t on[] = {PP_POWER_SEQ_SOURCE_ON, PP_POWER_SEQ_AMPERE_ON};
    for (size_t i = 0u; i < 2u; i++) {
        setUp();
        TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(on[i]));
        uint64_t output = when(PP_ACT_OUTPUT_ON);
        for (size_t k = 0u; k < rec.count; k++) {
            uint8_t action = rec.log[k].action;
            if ((action == PP_ACT_SOURCE_PAIR_ON) || (action == PP_ACT_AMPERE_PAIR_ON) ||
                (action == PP_ACT_SMU_ON)) {
                TEST_ASSERT_LESS_THAN_UINT64(output, rec.log[k].at);
            }
        }
    }
    static const pp_power_seq_id_t off[] = {PP_POWER_SEQ_SOURCE_OFF, PP_POWER_SEQ_AMPERE_OFF};
    for (size_t i = 0u; i < 2u; i++) {
        setUp();
        TEST_ASSERT_EQUAL(PP_SEQ_DONE, run(off[i]));
        TEST_ASSERT_EQUAL_UINT8(PP_ACT_OUTPUT_OFF, rec.log[0].action);
    }
}

static void test_an_unknown_sequence_has_no_table(void)
{
    size_t count = 99u;
    TEST_ASSERT_NULL(pp_power_seq(PP_POWER_SEQ_COUNT, &count));
    TEST_ASSERT_EQUAL_size_t(0u, count);
    TEST_ASSERT_NULL(pp_power_seq((pp_power_seq_id_t)-1, &count));
    /* The length is optional. */
    TEST_ASSERT_NOT_NULL(pp_power_seq(PP_POWER_SEQ_SOURCE_ON, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_source_on_follows_the_order_of_f28);
    RUN_TEST(test_source_on_waits_the_times_of_f28);
    RUN_TEST(test_source_on_does_nothing_when_its_conditions_do_not_hold);
    RUN_TEST(test_source_on_fails_when_the_output_is_not_at_the_set_point);
    RUN_TEST(test_smu_on_is_set_in_one_place_only);
    RUN_TEST(test_a_sequence_raises_one_mode_request_only);
    RUN_TEST(test_ampere_on_follows_the_order_and_times_of_f24);
    RUN_TEST(test_ampere_on_does_nothing_without_a_valid_vin);
    RUN_TEST(test_ampere_on_waits_for_the_ladder_node);
    RUN_TEST(test_ampere_on_fails_after_3_seconds_with_the_ladder_node_high);
    RUN_TEST(test_source_off_follows_the_order_of_f29);
    RUN_TEST(test_source_off_follows_a_dut_above_the_set_point_first);
    RUN_TEST(test_ampere_off_opens_the_output_first);
    RUN_TEST(test_the_output_pair_closes_last_and_opens_first);
    RUN_TEST(test_an_unknown_sequence_has_no_table);
    return UNITY_END();
}
