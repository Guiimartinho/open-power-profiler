/*
 * Timed sequences as data.
 *
 * The rules of section 6.6 of the specification fix the order and the times
 * in which the firmware starts the carrier, closes a path and opens it
 * again. Each such order is a table of steps, and this runner walks one
 * table without ever blocking: the loop of the application polls it, and a
 * poll does what is due and returns.
 *
 * A step is, in this order:
 *
 *   1. a guard: when it is given and does not hold, the step is skipped;
 *   2. an action, done once;
 *   3. a wait, the shortest time before anything else happens;
 *   4. a check: when it fails it is tried again on every poll for the retry
 *      time of the step, and after that the sequence has failed with the
 *      cause of the step.
 *
 * The runner knows no action and no check: both are numbers that the owner
 * of a table defines and that the operations it supplies carry out. Zero
 * means none. A test replaces the operations by a recorder and reads a table
 * back against the text of its rule.
 */
#ifndef PP_SEQUENCE_H
#define PP_SEQUENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "base/pp_status.h"

typedef struct {
    uint8_t guard;     /* check that must hold for the step to run, or zero */
    uint8_t action;    /* what to do, or zero */
    uint32_t wait_us;  /* shortest time between the action and the check */
    uint8_t check;     /* what must hold after the wait, or zero */
    uint32_t retry_us; /* how long a failing check is tried again */
    uint8_t fault;     /* cause reported when the check fails */
} pp_step_t;

typedef struct {
    void *ctx;
    void (*act)(void *ctx, uint8_t action);
    bool (*check)(void *ctx, uint8_t check);
} pp_seq_ops_t;

typedef enum {
    PP_SEQ_IDLE = 0, /* nothing started, or aborted */
    PP_SEQ_RUNNING,
    PP_SEQ_DONE,
    PP_SEQ_FAILED,
} pp_seq_state_t;

typedef struct {
    pp_seq_ops_t ops;
    const pp_step_t *steps;
    size_t count;
    size_t index;    /* step in progress */
    bool entered;    /* the action of the step in progress is done */
    uint64_t due_us; /* end of the wait of the step in progress */
    pp_seq_state_t state;
} pp_seq_t;

/* Prepare a runner. Both operations are required. */
pp_status_t pp_seq_init(pp_seq_t *seq, const pp_seq_ops_t *ops);

/*
 * Start a table. Nothing is done before the first poll. Returns
 * PP_ERR_INVALID_STATE while another table is running.
 */
pp_status_t pp_seq_start(pp_seq_t *seq, const pp_step_t *steps, size_t count);

/*
 * Do what is due at the time `now_us` and return the state. Steps without a
 * wait run in the same poll. A runner that is not running does nothing.
 */
pp_seq_state_t pp_seq_poll(pp_seq_t *seq, uint64_t now_us);

/* Stop where the sequence is. No action is undone: the caller decides. */
void pp_seq_abort(pp_seq_t *seq);

/* State without polling. PP_SEQ_IDLE for a NULL runner. */
pp_seq_state_t pp_seq_state(const pp_seq_t *seq);

/* Index of the step in progress, or of the step that failed. */
size_t pp_seq_step(const pp_seq_t *seq);

/* Cause of the step that failed. Meaningful in PP_SEQ_FAILED only. */
uint8_t pp_seq_fault(const pp_seq_t *seq);

#endif /* PP_SEQUENCE_H */
