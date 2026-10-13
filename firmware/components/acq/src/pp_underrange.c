#include "acq/pp_underrange.h"

#include <stddef.h>

pp_status_t pp_underrange_init(pp_underrange_t *rule, const pp_underrange_config_t *config)
{
    if ((rule == NULL) || (config == NULL) || (config->qualify == 0u) || (config->fault == 0u)) {
        return PP_ERR_INVALID_ARG;
    }
    rule->config = *config;
    pp_underrange_reset(rule);
    return PP_OK;
}

pp_status_t pp_underrange_init_default(pp_underrange_t *rule)
{
    const pp_underrange_config_t config = {
        .threshold = PP_UNDERRANGE_THRESHOLD,
        .qualify = PP_UNDERRANGE_QUALIFY,
        .hold = PP_UNDERRANGE_HOLD,
        .fault = PP_UNDERRANGE_FAULT,
    };
    return pp_underrange_init(rule, &config);
}

void pp_underrange_reset(pp_underrange_t *rule)
{
    if (rule == NULL) {
        return;
    }
    rule->run = 0u;
    rule->retry = 0u;
    rule->hold_left = 0u;
    rule->top_run = 0u;
    rule->flagged = false;
    rule->faulted = false;
}

/* The sample is not below the threshold: the flag clears and the hold starts. */
static void sample_above(pp_underrange_t *rule)
{
    if (rule->flagged) {
        rule->flagged = false;
        rule->hold_left = rule->config.hold;
    } else if (rule->hold_left > 0u) {
        rule->hold_left--;
    }
    rule->run = 0u;
    rule->top_run = 0u;
    rule->faulted = false;
}

/* The request for range 3: at the rise of the flag, then once per qualify time. */
static unsigned request_top_range(pp_underrange_t *rule, uint8_t range)
{
    if (range == PP_UNDERRANGE_TOP_RANGE) {
        rule->retry = 0u;
        return 0u;
    }
    if (rule->retry > 0u) {
        rule->retry--;
        return 0u;
    }
    rule->retry = (uint16_t)(rule->config.qualify - 1u);
    return (unsigned)PP_UNDER_REQUEST_R3;
}

/* The fault: once, when the code has been low in range 3 for the fault time. */
static unsigned count_top_range(pp_underrange_t *rule, uint8_t range)
{
    if (range != PP_UNDERRANGE_TOP_RANGE) {
        rule->top_run = 0u;
        return 0u;
    }
    if (rule->top_run < rule->config.fault) {
        rule->top_run++;
    }
    if ((rule->top_run >= rule->config.fault) && !rule->faulted) {
        rule->faulted = true;
        return (unsigned)PP_UNDER_FAULT;
    }
    return 0u;
}

unsigned pp_underrange_update(pp_underrange_t *rule, uint16_t adc, uint8_t range, bool path_closed)
{
    if (rule == NULL) {
        return 0u;
    }
    if (!path_closed) {
        pp_underrange_reset(rule);
        return 0u;
    }
    if (adc >= rule->config.threshold) {
        sample_above(rule);
        return 0u;
    }

    unsigned result = count_top_range(rule, range);
    if (rule->run < rule->config.qualify) {
        rule->run++;
    }
    if (rule->run >= rule->config.qualify) {
        if (!rule->flagged) {
            rule->flagged = true;
            rule->retry = 0u;
        }
        result |= (unsigned)PP_UNDER_MARK;
        result |= request_top_range(rule, range);
    }
    return result;
}

bool pp_underrange_blocks_step_down(const pp_underrange_t *rule)
{
    return (rule != NULL) && (rule->flagged || (rule->hold_left > 0u));
}
