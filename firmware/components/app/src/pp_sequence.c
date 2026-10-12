#include "app/pp_sequence.h"

pp_status_t pp_seq_init(pp_seq_t *seq, const pp_seq_ops_t *ops)
{
    if ((seq == NULL) || (ops == NULL) || (ops->act == NULL) || (ops->check == NULL)) {
        return PP_ERR_INVALID_ARG;
    }
    seq->ops = *ops;
    seq->steps = NULL;
    seq->count = 0u;
    seq->index = 0u;
    seq->entered = false;
    seq->due_us = 0u;
    seq->state = PP_SEQ_IDLE;
    return PP_OK;
}

pp_status_t pp_seq_start(pp_seq_t *seq, const pp_step_t *steps, size_t count)
{
    if ((seq == NULL) || ((steps == NULL) && (count > 0u))) {
        return PP_ERR_INVALID_ARG;
    }
    if (seq->state == PP_SEQ_RUNNING) {
        return PP_ERR_INVALID_STATE;
    }
    seq->steps = steps;
    seq->count = count;
    seq->index = 0u;
    seq->entered = false;
    seq->due_us = 0u;
    seq->state = PP_SEQ_RUNNING;
    return PP_OK;
}

/* Guard and action of the step in progress. Returns false when it is skipped. */
static bool enter_step(pp_seq_t *seq, const pp_step_t *step, uint64_t now_us)
{
    if ((step->guard != 0u) && !seq->ops.check(seq->ops.ctx, step->guard)) {
        return false;
    }
    if (step->action != 0u) {
        seq->ops.act(seq->ops.ctx, step->action);
    }
    seq->due_us = now_us + step->wait_us;
    seq->entered = true;
    return true;
}

pp_seq_state_t pp_seq_poll(pp_seq_t *seq, uint64_t now_us)
{
    if (seq == NULL) {
        return PP_SEQ_IDLE;
    }
    while (seq->state == PP_SEQ_RUNNING) {
        if (seq->index >= seq->count) {
            seq->state = PP_SEQ_DONE;
            break;
        }
        const pp_step_t *step = &seq->steps[seq->index];
        if (!seq->entered && !enter_step(seq, step, now_us)) {
            seq->index++;
            continue;
        }
        if (now_us < seq->due_us) {
            break; /* still waiting */
        }
        if ((step->check != 0u) && !seq->ops.check(seq->ops.ctx, step->check)) {
            if ((now_us - seq->due_us) >= step->retry_us) {
                seq->state = PP_SEQ_FAILED;
            }
            break; /* failed, or tried again at the next poll */
        }
        seq->index++;
        seq->entered = false;
    }
    return seq->state;
}

void pp_seq_abort(pp_seq_t *seq)
{
    if (seq == NULL) {
        return;
    }
    seq->state = PP_SEQ_IDLE;
    seq->entered = false;
}

pp_seq_state_t pp_seq_state(const pp_seq_t *seq)
{
    return (seq == NULL) ? PP_SEQ_IDLE : seq->state;
}

size_t pp_seq_step(const pp_seq_t *seq)
{
    return (seq == NULL) ? 0u : seq->index;
}

uint8_t pp_seq_fault(const pp_seq_t *seq)
{
    if ((seq == NULL) || (seq->state != PP_SEQ_FAILED)) {
        return 0u;
    }
    return seq->steps[seq->index].fault;
}
