#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app/pp_sequence.h"

#define LOG_SIZE 32u
#define IDS 8u

/* Operations that write down every call with the time it was made at. */
typedef struct {
    uint64_t now;
    struct {
        uint64_t at;
        char kind; /* 'A' for an action, 'C' for a check */
        uint8_t id;
    } log[LOG_SIZE];
    size_t count;
    bool holds[IDS]; /* result of each check */
} recorder_t;

static recorder_t rec;
static pp_seq_t seq;

static void note(recorder_t *r, char kind, uint8_t id)
{
    TEST_ASSERT_LESS_THAN_size_t(LOG_SIZE, r->count);
    r->log[r->count].at = r->now;
    r->log[r->count].kind = kind;
    r->log[r->count].id = id;
    r->count++;
}

static void rec_act(void *ctx, uint8_t action)
{
    note((recorder_t *)ctx, 'A', action);
}

static bool rec_check(void *ctx, uint8_t check)
{
    recorder_t *r = (recorder_t *)ctx;
    note(r, 'C', check);
    return r->holds[check];
}

static pp_seq_state_t poll_at(uint64_t now)
{
    rec.now = now;
    return pp_seq_poll(&seq, now);
}

/* How many calls of one kind and number are in the log. */
static size_t calls(char kind, uint8_t id)
{
    size_t found = 0u;
    for (size_t i = 0u; i < rec.count; i++) {
        if ((rec.log[i].kind == kind) && (rec.log[i].id == id)) {
            found++;
        }
    }
    return found;
}

void setUp(void)
{
    memset(&rec, 0, sizeof(rec));
    for (size_t i = 0u; i < IDS; i++) {
        rec.holds[i] = true;
    }
    const pp_seq_ops_t ops = {.ctx = &rec, .act = rec_act, .check = rec_check};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_init(&seq, &ops));
}

void tearDown(void)
{
}

static void test_sequence_init_checks_its_arguments(void)
{
    pp_seq_t other;
    const pp_seq_ops_t no_act = {.ctx = &rec, .act = NULL, .check = rec_check};
    const pp_seq_ops_t no_check = {.ctx = &rec, .act = rec_act, .check = NULL};
    const pp_seq_ops_t good = {.ctx = &rec, .act = rec_act, .check = rec_check};
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_init(NULL, &good));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_init(&other, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_init(&other, &no_act));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_init(&other, &no_check));
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, pp_seq_state(&seq));
}

static void test_sequence_start_checks_its_arguments(void)
{
    static const pp_step_t steps[] = {{.action = 1u}};
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_start(NULL, steps, 1u));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_seq_start(&seq, NULL, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, pp_seq_state(&seq));
}

static void test_sequence_nothing_happens_before_the_first_poll(void)
{
    static const pp_step_t steps[] = {{.action = 1u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, pp_seq_state(&seq));
    TEST_ASSERT_EQUAL_size_t(0u, rec.count);
}

static void test_sequence_an_empty_table_is_done_at_once(void)
{
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, NULL, 0u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(0u));
    TEST_ASSERT_EQUAL_size_t(0u, rec.count);
}

static void test_sequence_steps_without_a_wait_run_in_one_poll_in_order(void)
{
    static const pp_step_t steps[] = {{.action = 3u}, {.action = 1u}, {.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 3u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(100u));
    TEST_ASSERT_EQUAL_size_t(3u, rec.count);
    TEST_ASSERT_EQUAL_UINT8(3u, rec.log[0].id);
    TEST_ASSERT_EQUAL_UINT8(1u, rec.log[1].id);
    TEST_ASSERT_EQUAL_UINT8(2u, rec.log[2].id);
}

static void test_sequence_the_next_step_waits_for_the_time_of_the_one_before(void)
{
    static const pp_step_t steps[] = {{.action = 1u, .wait_us = 5000u}, {.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(1000u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(3000u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(5999u));
    TEST_ASSERT_EQUAL_size_t(1u, rec.count); /* the action is done once */
    TEST_ASSERT_EQUAL_size_t(0u, pp_seq_step(&seq));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(6000u));
    TEST_ASSERT_EQUAL_size_t(2u, rec.count);
    TEST_ASSERT_EQUAL_UINT64(1000u, rec.log[0].at);
    TEST_ASSERT_EQUAL_UINT64(6000u, rec.log[1].at);
}

static void test_sequence_a_check_is_made_after_the_wait_and_not_before(void)
{
    static const pp_step_t steps[] = {{.action = 1u, .wait_us = 200u, .check = 4u}, {.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(199u));
    TEST_ASSERT_EQUAL_size_t(0u, calls('C', 4u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(200u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('C', 4u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 2u));
}

static void test_sequence_a_failed_check_ends_the_sequence_with_its_cause(void)
{
    static const pp_step_t steps[] = {
        {.action = 1u}, {.action = 2u, .wait_us = 10u, .check = 5u, .fault = 9u}, {.action = 3u}};
    rec.holds[5] = false;
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 3u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL_UINT8(0u, pp_seq_fault(&seq)); /* no cause while running */
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, poll_at(10u));
    TEST_ASSERT_EQUAL_UINT8(9u, pp_seq_fault(&seq));
    TEST_ASSERT_EQUAL_size_t(1u, pp_seq_step(&seq));
    TEST_ASSERT_EQUAL_size_t(0u, calls('A', 3u));

    /* A failed sequence does nothing more. */
    size_t before = rec.count;
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, poll_at(1000u));
    TEST_ASSERT_EQUAL_size_t(before, rec.count);
}

static void test_sequence_a_check_is_tried_again_for_its_retry_time(void)
{
    static const pp_step_t steps[] = {
        {.action = 1u, .wait_us = 100u, .check = 6u, .retry_us = 3000u, .fault = 7u},
        {.action = 2u}};
    rec.holds[6] = false;
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(100u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(2000u));
    TEST_ASSERT_EQUAL_size_t(2u, calls('C', 6u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 1u)); /* the action is not repeated */

    rec.holds[6] = true;
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(2500u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 2u));
}

static void test_sequence_a_check_that_never_holds_fails_when_the_retry_time_is_over(void)
{
    static const pp_step_t steps[] = {
        {.action = 1u, .wait_us = 100u, .check = 6u, .retry_us = 3000u, .fault = 7u}};
    rec.holds[6] = false;
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(3099u));
    TEST_ASSERT_EQUAL(PP_SEQ_FAILED, poll_at(3100u));
    TEST_ASSERT_EQUAL_UINT8(7u, pp_seq_fault(&seq));
}

static void test_sequence_a_guard_that_does_not_hold_skips_its_step(void)
{
    static const pp_step_t steps[] = {
        {.guard = 3u, .action = 1u, .wait_us = 30000u, .check = 4u, .fault = 5u}, {.action = 2u}};
    rec.holds[3] = false;
    rec.holds[4] = false; /* would fail if the step ran */
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(0u));
    TEST_ASSERT_EQUAL_size_t(0u, calls('A', 1u));
    TEST_ASSERT_EQUAL_size_t(0u, calls('C', 4u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 2u));
}

static void test_sequence_a_guard_that_holds_lets_its_step_run(void)
{
    static const pp_step_t steps[] = {{.guard = 3u, .action = 1u, .wait_us = 30000u},
                                      {.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('C', 3u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(29999u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('C', 3u)); /* the guard is asked once */
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(30000u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 1u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 2u));
}

static void test_sequence_a_running_table_cannot_be_replaced(void)
{
    static const pp_step_t first[] = {{.action = 1u, .wait_us = 50u}};
    static const pp_step_t second[] = {{.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, first, 1u));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_STATE, pp_seq_start(&seq, second, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_STATE, pp_seq_start(&seq, second, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(50u));
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, second, 1u));
    TEST_ASSERT_EQUAL(PP_SEQ_DONE, poll_at(60u));
    TEST_ASSERT_EQUAL_size_t(1u, calls('A', 2u));
}

static void test_sequence_abort_stops_where_it_is(void)
{
    static const pp_step_t steps[] = {{.action = 1u, .wait_us = 50u}, {.action = 2u}};
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(0u));
    pp_seq_abort(&seq);
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, pp_seq_state(&seq));
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, poll_at(1000u));
    TEST_ASSERT_EQUAL_size_t(0u, calls('A', 2u));

    /* The runner can start again, from the first step. */
    TEST_ASSERT_EQUAL(PP_OK, pp_seq_start(&seq, steps, 2u));
    TEST_ASSERT_EQUAL(PP_SEQ_RUNNING, poll_at(2000u));
    TEST_ASSERT_EQUAL_size_t(2u, calls('A', 1u));
}

static void test_sequence_functions_tolerate_null(void)
{
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, pp_seq_poll(NULL, 0u));
    TEST_ASSERT_EQUAL(PP_SEQ_IDLE, pp_seq_state(NULL));
    TEST_ASSERT_EQUAL_size_t(0u, pp_seq_step(NULL));
    TEST_ASSERT_EQUAL_UINT8(0u, pp_seq_fault(NULL));
    pp_seq_abort(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sequence_init_checks_its_arguments);
    RUN_TEST(test_sequence_start_checks_its_arguments);
    RUN_TEST(test_sequence_nothing_happens_before_the_first_poll);
    RUN_TEST(test_sequence_an_empty_table_is_done_at_once);
    RUN_TEST(test_sequence_steps_without_a_wait_run_in_one_poll_in_order);
    RUN_TEST(test_sequence_the_next_step_waits_for_the_time_of_the_one_before);
    RUN_TEST(test_sequence_a_check_is_made_after_the_wait_and_not_before);
    RUN_TEST(test_sequence_a_failed_check_ends_the_sequence_with_its_cause);
    RUN_TEST(test_sequence_a_check_is_tried_again_for_its_retry_time);
    RUN_TEST(test_sequence_a_check_that_never_holds_fails_when_the_retry_time_is_over);
    RUN_TEST(test_sequence_a_guard_that_does_not_hold_skips_its_step);
    RUN_TEST(test_sequence_a_guard_that_holds_lets_its_step_run);
    RUN_TEST(test_sequence_a_running_table_cannot_be_replaced);
    RUN_TEST(test_sequence_abort_stops_where_it_is);
    RUN_TEST(test_sequence_functions_tolerate_null);
    return UNITY_END();
}
