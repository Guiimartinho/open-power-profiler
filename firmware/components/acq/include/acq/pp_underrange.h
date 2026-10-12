/*
 * Under-range rule (section 4.4 of the specification, rule F-22).
 *
 * The comparators see forward current only. A current that flows backward
 * shows as a code below the zero of the range, and the rule watches for it
 * while a mode pair is closed:
 *
 *   - A code below the threshold for a number of samples in a row raises the
 *     flag. Every sample is marked under-range from there until the first
 *     one that is not below the threshold.
 *   - When the flag rises, range 3 is requested at once, also with the range
 *     locked. The request is repeated while the flag is up and the samples
 *     still show another range.
 *   - No step down is requested while the flag is up, nor for a hold time
 *     after it cleared.
 *   - A code that stays below the threshold in range 3 for the fault time is
 *     a reverse current: the caller opens the output and reports the fault.
 *
 * The rule works on samples, so its times are counts of samples: the caller
 * converts them from the sample rate. It keeps no thresholds of its own; the
 * numbers of F-22 are the defaults below.
 */
#ifndef PP_UNDERRANGE_H
#define PP_UNDERRANGE_H

#include <stdbool.h>
#include <stdint.h>

#include "base/pp_status.h"

/* Numbers of F-22 at 100 kSPS. */
#define PP_UNDERRANGE_THRESHOLD 650u /* codes; half the pedestal */
#define PP_UNDERRANGE_QUALIFY 100u   /* samples below it before the flag */
#define PP_UNDERRANGE_HOLD 100000u   /* 1 s without a step down */
#define PP_UNDERRANGE_FAULT 10000u   /* 100 ms below it in range 3 */
#define PP_UNDERRANGE_TOP_RANGE 3u   /* the range that is requested */

/* What a sample means. The values combine. */
enum {
    PP_UNDER_MARK = 1u << 0,       /* mark the sample under-range */
    PP_UNDER_REQUEST_R3 = 1u << 1, /* ask the sequencer for range 3 now */
    PP_UNDER_FAULT = 1u << 2,      /* reverse current: open the output */
};

typedef struct {
    uint16_t threshold; /* a code below this is under-range */
    uint16_t qualify;   /* samples in a row before the flag rises, at least 1 */
    uint32_t hold;      /* samples without a step down after the flag cleared */
    uint32_t fault;     /* samples below the threshold in range 3 before the fault */
} pp_underrange_config_t;

typedef struct {
    pp_underrange_config_t config;
    uint16_t run;       /* samples below the threshold in a row, up to qualify */
    uint16_t retry;     /* samples until range 3 is requested again */
    uint32_t hold_left; /* samples left of the hold after the flag cleared */
    uint32_t top_run;   /* samples below the threshold in range 3 in a row */
    bool flagged;       /* the flag is up */
    bool faulted;       /* the fault of this episode was reported */
} pp_underrange_t;

/* Prepare the rule. Returns PP_ERR_INVALID_ARG when qualify or fault is zero. */
pp_status_t pp_underrange_init(pp_underrange_t *rule, const pp_underrange_config_t *config);

/* Prepare the rule with the numbers of F-22. */
pp_status_t pp_underrange_init_default(pp_underrange_t *rule);

/* Start over, for instance when a path closes. The configuration is kept. */
void pp_underrange_reset(pp_underrange_t *rule);

/*
 * Process one sample: its code, its range bits, and whether a mode pair is
 * closed. With no pair closed the rule rests and forgets what it counted.
 * Returns a combination of the PP_UNDER_ values.
 */
unsigned pp_underrange_update(pp_underrange_t *rule, uint16_t adc, uint8_t range, bool path_closed);

/* True while no step down may be requested: flag up, or hold running. */
bool pp_underrange_blocks_step_down(const pp_underrange_t *rule);

#endif /* PP_UNDERRANGE_H */
