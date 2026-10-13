/*
 * Slow monitor channels (section 4.2 of the specification) and the rules
 * that read them (F-10 to F-12 of section 6.6).
 *
 * An 8-channel, 12-bit converter with the 2.5 V reference reads eight
 * signals at 100 SPS each. This module turns codes into millivolts with the
 * nominal scale factors of the specification, tells which input supplies
 * the 5 V rail, and runs the rails test. It is arithmetic on a scan: the
 * adapter that talks to the converter is elsewhere.
 *
 * All scale factors are calculated from nominal resistor values. None is
 * measured, and the temperature scale is the nominal one of a candidate
 * part.
 */
#ifndef PP_MONITOR_H
#define PP_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

#include "proto/pp_proto_defs.h"

#define PP_MON_CHANNELS 8u
#define PP_MON_CODE_MAX 4095u

/* Channels of the monitor converter (section 6.6). */
enum {
    PP_MON_CH_VOUT = 0, /* ladder output, buffered */
    PP_MON_CH_VIN = 1,  /* VIN behind the fuse */
    PP_MON_CH_RAIL5 = 2,
    PP_MON_CH_CC1 = 3,
    PP_MON_CH_CC2 = 4,
    PP_MON_CH_TEMP = 5,
    PP_MON_CH_P12 = 6, /* +12 V_A */
    PP_MON_CH_N4 = 7,  /* -4 V_A */
};

/* One reading of every channel. */
typedef struct {
    uint16_t code[PP_MON_CHANNELS];
    bool answered; /* false when the converter gave no plausible frame */
} pp_mon_scan_t;

/* Which input supplies the 5 V rail (F-11) is a pp_supply_t of the protocol. */
typedef struct {
    pp_supply_t input;
    int32_t millivolts; /* the 5 V rail */
    bool consistent;    /* false when the reading fits neither input */
} pp_mon_rail5_t;

/* Voltage at the converter pin for a code, in microvolts. */
uint32_t pp_mon_pin_microvolts(uint16_t code);

/* Ladder output (channel 0), in millivolts. */
int32_t pp_mon_vout_mv(uint16_t code);

/*
 * VIN (channel 1), in millivolts: the reading times 44, less the offset of
 * the channel stored at calibration (F-26).
 */
int32_t pp_mon_vin_mv(uint16_t code, int32_t offset_mv);

/*
 * The 5 V rail and its source (channel 2, F-11). Above 1.67 V at the pin the
 * USB-C input supplies. At or below it the module supplies; a result above
 * 5.7 V is then inconsistent, and the caller treats the rail as too low.
 */
pp_mon_rail5_t pp_mon_rail5(uint16_t code);

/* A CC pin (channels 3 and 4), in millivolts. */
int32_t pp_mon_cc_mv(uint16_t code);

/* Board temperature (channel 5), in tenths of a degree Celsius. */
int32_t pp_mon_temp_decidegrees(uint16_t code);

/* +12 V_A (channel 6), in millivolts. */
int32_t pp_mon_p12_mv(uint16_t code);

/* -4 V_A (channel 7), in millivolts; the value is negative. */
int32_t pp_mon_n4_mv(uint16_t code);

/* Reasons for which the rails test of F-12 fails. They can combine. */
enum {
    PP_RAILS_NO_ANSWER = 1u << 0,  /* the converter does not answer */
    PP_RAILS_PWR_GOOD = 1u << 1,   /* PWR_GOOD is not valid (F-2) */
    PP_RAILS_P12 = 1u << 2,        /* +12 V_A outside 11.4 V to 12.6 V */
    PP_RAILS_N4 = 1u << 3,         /* -4 V_A outside -4.3 V to -3.7 V */
    PP_RAILS_RAIL5_HIGH = 1u << 4, /* 5 V rail at 5.50 V or more */
    PP_RAILS_RAIL5_LOW = 1u << 5,  /* 5 V rail below 4.25 V, or inconsistent */
    PP_RAILS_TEMP = 1u << 6,       /* temperature channel outside 0.3 V to 1.5 V */
    PP_RAILS_STUCK = 1u << 7,      /* a rail channel at code 0 or 4095 */
};

/*
 * The rails test of F-12 on one scan. `rail5_average_mv` is the 5 V rail
 * averaged over 10 ms by the caller. Returns zero when every check passes,
 * otherwise the reasons.
 */
uint32_t pp_mon_rails_test(const pp_mon_scan_t *scan, bool pwr_good_valid,
                           int32_t rail5_average_mv);

/*
 * True when the reasons set the rails flag. A 5 V rail that is too low and
 * nothing else is the power-limit fault of F-14 instead, and this is false.
 */
bool pp_mon_rails_flag(uint32_t reasons);

#endif /* PP_MONITOR_H */
