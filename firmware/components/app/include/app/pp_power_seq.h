/*
 * The sequences that switch the DUT on and off (section 6.4 of the
 * specification, rules F-23 to F-31 of section 6.6).
 *
 * Each sequence is a table of steps for the runner of pp_sequence.h. The
 * tables are the only place that holds the order and the times of these
 * rules, and a unit test reads them back against the text of the rules.
 *
 * The actions and checks are carried out by the operations that the
 * application supplies: they reach the DAC, the pins, the range sequencer
 * and the monitor through their ports.
 */
#ifndef PP_POWER_SEQ_H
#define PP_POWER_SEQ_H

#include <stddef.h>

#include "app/pp_sequence.h"

/* What a step does. Zero is "nothing", as the runner defines it. */
typedef enum {
    PP_ACT_NONE = 0,
    PP_ACT_DAC_START,       /* DAC to the code of 0.80 V (F-28) */
    PP_ACT_DAC_WORKING,     /* DAC to the working value */
    PP_ACT_DAC_FOLLOW,      /* DAC to the reading of channel 0 less 0.2 V (F-31) */
    PP_ACT_DAC_ZERO,        /* DAC to code zero, the idle state (F-30) */
    PP_ACT_SMU_ON,          /* pre-regulator enabled */
    PP_ACT_SMU_OFF,         /* pre-regulator disabled */
    PP_ACT_RANGE3,          /* range 3 selected (F-20) */
    PP_ACT_SOURCE_PAIR_ON,  /* GATE_SRC high */
    PP_ACT_SOURCE_PAIR_OFF, /* GATE_SRC low */
    PP_ACT_AMPERE_PAIR_ON,  /* GATE_AMP high */
    PP_ACT_AMPERE_PAIR_OFF, /* GATE_AMP low */
    PP_ACT_OUTPUT_ON,       /* output switch on, through the trip program */
    PP_ACT_OUTPUT_OFF,      /* output switch off */
    PP_ACT_RANGING_RELEASE, /* automatic ranging, or the locked range, takes over */
    PP_ACT_COUNT,
} pp_power_action_t;

/* What a step asks. Zero is "nothing". */
typedef enum {
    PP_CHK_NONE = 0,
    PP_CHK_SOURCE_READY,       /* PWR_GOOD valid, channel 7 in its window, SMU_ON low (F-28) */
    PP_CHK_VOUT_AT_SETPOINT,   /* channel 0 within 100 mV of the set-point (F-28) */
    PP_CHK_VIN_VALID,          /* VIN_OV low, VIN inside 0.6 V to 5.4 V for 100 ms (F-26) */
    PP_CHK_LADDER_BELOW_VIN,   /* channel 0 below VIN plus 0.1 V, or below 0.3 V (F-25) */
    PP_CHK_DUT_ABOVE_SETPOINT, /* channel 0 more than 0.3 V above the DAC value (F-31) */
    PP_CHK_COUNT,
} pp_power_check_t;

typedef enum {
    PP_POWER_SEQ_SOURCE_ON = 0, /* DUT power on in source mode (F-28, F-24) */
    PP_POWER_SEQ_SOURCE_OFF,    /* DUT power off in source mode (F-29, F-31) */
    PP_POWER_SEQ_AMPERE_ON,     /* DUT power on in ampere mode (F-24 to F-26) */
    PP_POWER_SEQ_AMPERE_OFF,    /* DUT power off in ampere mode (F-23) */
    PP_POWER_SEQ_COUNT,
} pp_power_seq_id_t;

/* Times of the rules, in microseconds. */
#define PP_T_PRECHARGE_US 200000u    /* DAC at 0.80 V before SMU_ON (F-28) */
#define PP_T_ENABLE_US 5000u         /* SMU_ON before the working value (F-28) */
#define PP_T_SETPOINT_US 80000u      /* working value before the source pair (F-28) */
#define PP_T_SOURCE_PAIR_US 60000u   /* source pair before its check (F-28) */
#define PP_T_MODE_PAIR_US 40000u     /* a mode pair counts as closed (F-24) */
#define PP_T_OUTPUT_US 50000u        /* the output counts as on (F-24) */
#define PP_T_LADDER_WAIT_US 3000000u /* longest wait for the ladder node (F-25) */
#define PP_T_FOLLOW_US 30000u        /* DAC lowered before a switch opens (F-31) */
#define PP_T_MODE_LOCK_US 5000u      /* after a mode request fell (F-23) */

/* The table of a sequence and its length. NULL and zero for an unknown one. */
const pp_step_t *pp_power_seq(pp_power_seq_id_t id, size_t *count);

#endif /* PP_POWER_SEQ_H */
