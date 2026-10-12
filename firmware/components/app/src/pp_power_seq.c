#include "app/pp_power_seq.h"

#include "app/pp_fault.h"

/*
 * F-28, the only start of the source, then the output as F-24 says. The
 * numbers in the comments are the steps of the rule.
 */
static const pp_step_t source_on[] = {
    /* Before (1): the conditions under which the sequence may start. */
    {.check = PP_CHK_SOURCE_READY, .fault = PP_CAUSE_START_CHECK},
    /* (1) and (2) */
    {.action = PP_ACT_DAC_START, .wait_us = PP_T_PRECHARGE_US},
    /* (3) and (4) */
    {.action = PP_ACT_SMU_ON, .wait_us = PP_T_ENABLE_US},
    /* (5) */
    {.action = PP_ACT_DAC_WORKING, .wait_us = PP_T_SETPOINT_US},
    /* (6): range 3 first, then the source pair */
    {.action = PP_ACT_RANGE3},
    /* (7) */
    {.action = PP_ACT_SOURCE_PAIR_ON,
     .wait_us = PP_T_SOURCE_PAIR_US,
     .check = PP_CHK_VOUT_AT_SETPOINT,
     .fault = PP_CAUSE_START_CHECK},
    /* (8): the output pair, last. It counts as on 50 ms later (F-24). */
    {.action = PP_ACT_OUTPUT_ON, .wait_us = PP_T_OUTPUT_US},
    {.action = PP_ACT_RANGING_RELEASE},
};

/*
 * F-29, with the step of F-31 in front: a DUT that holds the output above
 * the set-point is followed down before any switch opens.
 */
static const pp_step_t source_off[] = {
    {.guard = PP_CHK_DUT_ABOVE_SETPOINT, .action = PP_ACT_DAC_FOLLOW, .wait_us = PP_T_FOLLOW_US},
    {.action = PP_ACT_OUTPUT_OFF},
    {.action = PP_ACT_SOURCE_PAIR_OFF},
    {.action = PP_ACT_SMU_OFF},
    /* The 5 ms of F-23 run from the moment the request of the pair fell. */
    {.action = PP_ACT_DAC_ZERO, .wait_us = PP_T_MODE_LOCK_US},
};

/* F-26, F-20, F-25 and F-24, in that order. */
static const pp_step_t ampere_on[] = {
    {.check = PP_CHK_VIN_VALID, .fault = PP_CAUSE_VIN_RANGE},
    {.action = PP_ACT_RANGE3,
     .check = PP_CHK_LADDER_BELOW_VIN,
     .retry_us = PP_T_LADDER_WAIT_US,
     .fault = PP_CAUSE_START_CHECK},
    {.action = PP_ACT_AMPERE_PAIR_ON, .wait_us = PP_T_MODE_PAIR_US},
    {.action = PP_ACT_OUTPUT_ON, .wait_us = PP_T_OUTPUT_US},
    {.action = PP_ACT_RANGING_RELEASE},
};

/* F-23: the output pair first, then the mode pair, then 5 ms. */
static const pp_step_t ampere_off[] = {
    {.action = PP_ACT_OUTPUT_OFF},
    {.action = PP_ACT_AMPERE_PAIR_OFF, .wait_us = PP_T_MODE_LOCK_US},
};

#define LENGTH(table) (sizeof(table) / sizeof((table)[0]))

const pp_step_t *pp_power_seq(pp_power_seq_id_t id, size_t *count)
{
    const pp_step_t *steps = NULL;
    size_t length = 0u;
    switch (id) {
    case PP_POWER_SEQ_SOURCE_ON:
        steps = source_on;
        length = LENGTH(source_on);
        break;
    case PP_POWER_SEQ_SOURCE_OFF:
        steps = source_off;
        length = LENGTH(source_off);
        break;
    case PP_POWER_SEQ_AMPERE_ON:
        steps = ampere_on;
        length = LENGTH(ampere_on);
        break;
    case PP_POWER_SEQ_AMPERE_OFF:
        steps = ampere_off;
        length = LENGTH(ampere_off);
        break;
    case PP_POWER_SEQ_COUNT:
    default:
        break;
    }
    if (count != NULL) {
        *count = length;
    }
    return steps;
}
