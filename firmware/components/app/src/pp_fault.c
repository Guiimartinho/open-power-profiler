#include "app/pp_fault.h"

#include <stddef.h>

#include "proto/pp_proto_defs.h"

typedef struct {
    const char *name;
    uint16_t wire_flag; /* the flag of section 7.4 that reports the cause */
    bool report_only;
} cause_info_t;

/* One line per cause, in the order of pp_cause_t. Every cause has its flag. */
static const cause_info_t cause_table[PP_CAUSE_COUNT] = {
    [PP_CAUSE_OVERCURRENT] = {"over-current", PP_FAULT_OVERCURRENT, false},
    [PP_CAUSE_START_OVERCURRENT] = {"start over-current", PP_FAULT_START_OVERCURRENT, false},
    [PP_CAUSE_SEQUENCER] = {"sequencer", PP_FAULT_SEQUENCER, false},
    [PP_CAUSE_REVERSE_CURRENT] = {"reverse current", PP_FAULT_REVERSE_CURRENT, false},
    [PP_CAUSE_CARRIER_SUPPLY] = {"carrier supply", PP_FAULT_CARRIER_SUPPLY, false},
    [PP_CAUSE_ANALOG_RAILS] = {"analog rails", PP_FAULT_ANALOG_RAILS, false},
    [PP_CAUSE_VIN_OVERVOLTAGE] = {"VIN over-voltage", PP_FAULT_VIN_OVERVOLTAGE, false},
    [PP_CAUSE_VIN_RANGE] = {"VIN out of range", PP_FAULT_VIN_RANGE, false},
    [PP_CAUSE_POWER_LIMIT] = {"power limit", PP_FAULT_POWER_LIMIT, false},
    [PP_CAUSE_THERMAL] = {"thermal", PP_FAULT_THERMAL, false},
    [PP_CAUSE_SOURCE_OVERLOAD] = {"source overload", PP_FAULT_SOURCE_OVERLOAD, false},
    [PP_CAUSE_EXTERNAL_VOLTAGE] = {"external voltage", PP_FAULT_EXTERNAL_VOLTAGE, false},
    [PP_CAUSE_START_CHECK] = {"start check", PP_FAULT_START_CHECK, false},
    [PP_CAUSE_SELFTEST] = {"self-test", PP_FAULT_SELFTEST, false},
    [PP_CAUSE_WATCHDOG] = {"watchdog restart", PP_FAULT_WATCHDOG, true},
};

static bool cause_is_valid(pp_cause_t cause)
{
    return (unsigned)cause < (unsigned)PP_CAUSE_COUNT;
}

uint32_t pp_cause_bit(pp_cause_t cause)
{
    return cause_is_valid(cause) ? ((uint32_t)1u << (unsigned)cause) : 0u;
}

uint32_t pp_cause_all(void)
{
    return ((uint32_t)1u << (unsigned)PP_CAUSE_COUNT) - 1u;
}

bool pp_cause_is_report_only(pp_cause_t cause)
{
    return cause_is_valid(cause) && cause_table[cause].report_only;
}

bool pp_cause_needs_fault_state(uint32_t causes)
{
    for (unsigned i = 0u; i < (unsigned)PP_CAUSE_COUNT; i++) {
        if (((causes & ((uint32_t)1u << i)) != 0u) && !cause_table[i].report_only) {
            return true;
        }
    }
    return false;
}

uint16_t pp_cause_wire_flags(uint32_t causes)
{
    uint16_t flags = 0u;
    for (unsigned i = 0u; i < (unsigned)PP_CAUSE_COUNT; i++) {
        if ((causes & ((uint32_t)1u << i)) != 0u) {
            flags = (uint16_t)(flags | cause_table[i].wire_flag);
        }
    }
    return flags;
}

const char *pp_cause_name(pp_cause_t cause)
{
    return cause_is_valid(cause) ? cause_table[cause].name : "unknown";
}
