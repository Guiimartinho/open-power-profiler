/*
 * Fault causes of the instrument (section 6.4 of the specification).
 *
 * Section 6.4 lists what raises a fault and section 6.6 gives each cause its
 * rule. The firmware keeps the causes as a set of bits, one per cause, which
 * stays latched until the host clears it.
 *
 * Since version 2 of the protocol every cause has a fault flag of its own
 * (section 7.4, D-103), and pp_cause_wire_flags() translates a set of causes
 * into those flags. A trip within 100 ms of GATE_OUT is reported with two
 * flags, over-current and start over-current: the caller raises both causes.
 */
#ifndef PP_FAULT_H
#define PP_FAULT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PP_CAUSE_OVERCURRENT = 0,   /* trip in range 3 (F-18, F-19) */
    PP_CAUSE_START_OVERCURRENT, /* trip within 100 ms of GATE_OUT (F-19) */
    PP_CAUSE_SEQUENCER,         /* range sequencer not acting (F-21) */
    PP_CAUSE_REVERSE_CURRENT,   /* current flowing backward (F-22) */
    PP_CAUSE_CARRIER_SUPPLY,    /* PWR_GOOD missing or lost (F-2, F-7) */
    PP_CAUSE_ANALOG_RAILS,      /* rails test failed (F-3, F-12) */
    PP_CAUSE_VIN_OVERVOLTAGE,   /* VIN above the over-voltage level (F-27) */
    PP_CAUSE_VIN_RANGE,         /* VIN absent, reversed or out of range (F-26) */
    PP_CAUSE_POWER_LIMIT,       /* input budget passed or 5 V rail low (F-14) */
    PP_CAUSE_THERMAL,           /* board temperature above its limit (F-15) */
    PP_CAUSE_SOURCE_OVERLOAD,   /* source output below its set-point (F-32) */
    PP_CAUSE_EXTERNAL_VOLTAGE,  /* voltage applied to the source output (F-33) */
    PP_CAUSE_START_CHECK,       /* a check of DUT power on failed (F-25, F-28) */
    PP_CAUSE_SELFTEST,          /* self-test failed */
    PP_CAUSE_WATCHDOG,          /* the last start was a watchdog reset (F-8) */
    PP_CAUSE_COUNT,
} pp_cause_t;

/* The bit of a cause in a set of causes; zero for a value that is no cause. */
uint32_t pp_cause_bit(pp_cause_t cause);

/* All causes that exist, as a set. */
uint32_t pp_cause_all(void);

/*
 * True for a cause that is only reported. The watchdog flag is one: it tells
 * the host what happened and does not keep the instrument in FAULT (F-8).
 */
bool pp_cause_is_report_only(pp_cause_t cause);

/* True when the set holds a cause that needs the FAULT state. */
bool pp_cause_needs_fault_state(uint32_t causes);

/* The fault flags of section 7.4 for a set of causes. */
uint16_t pp_cause_wire_flags(uint32_t causes);

/* Name of a cause, for logs. Never returns NULL. */
const char *pp_cause_name(pp_cause_t cause);

#endif /* PP_FAULT_H */
