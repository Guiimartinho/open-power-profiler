#include "monitor/pp_monitor.h"

#include <stddef.h>

/*
 * Scale factors of section 4.2, as the fraction of the signal that reaches
 * the converter, in parts per ten thousand.
 */
#define SCALE_VOUT 4545         /* channel 0 */
#define SCALE_RAIL5_USB_C 4545  /* channel 2 while the USB-C input supplies */
#define SCALE_RAIL5_MODULE 2524 /* channel 2 while the module supplies */
#define SCALE_P12 1754          /* channel 6 */

#define VIN_RATIO 44 /* channel 1: the terminal is 44 times the pin */

/* Channel 7 reads 0.333 of the rail plus 1.667 V. */
#define N4_OFFSET_UV 1667000
#define N4_SCALE_PER_MILLE 333

/* Nominal scale of the temperature sensor: 500 mV at 0 degrees, 10 mV per degree. */
#define TEMP_ZERO_UV 500000

/* F-11: above this level at the pin the USB-C input supplies. */
#define RAIL5_SOURCE_LIMIT_UV 1670000u
/* F-11: a rail above this, read on the module scale, is inconsistent. */
#define RAIL5_INCONSISTENT_MV 5700

/* Windows of F-12. */
#define P12_MIN_MV 11400
#define P12_MAX_MV 12600
#define N4_MIN_MV (-4300)
#define N4_MAX_MV (-3700)
#define RAIL5_MIN_MV 4250
#define RAIL5_LIMIT_MV 5500
#define TEMP_MIN_UV 300000u
#define TEMP_MAX_UV 1500000u

/* Quotient rounded to the nearest integer; the divisor is positive. */
static int32_t div_round(int64_t value, int64_t divisor)
{
    int64_t half = divisor / 2;
    int64_t biased = (value >= 0) ? (value + half) : (value - half);
    return (int32_t)(biased / divisor);
}

/* Signal in millivolts for a pin voltage and a scale in parts per ten thousand. */
static int32_t unscale_mv(uint32_t pin_uv, int32_t scale)
{
    return div_round((int64_t)pin_uv * 10, scale);
}

uint32_t pp_mon_pin_microvolts(uint16_t code)
{
    uint32_t limited = (code > PP_MON_CODE_MAX) ? PP_MON_CODE_MAX : code;
    /* 2.5 V over 4096 codes is 78125 / 128 microvolts per code. */
    return ((limited * 78125u) + 64u) / 128u;
}

int32_t pp_mon_vout_mv(uint16_t code)
{
    return unscale_mv(pp_mon_pin_microvolts(code), SCALE_VOUT);
}

int32_t pp_mon_vin_mv(uint16_t code, int32_t offset_mv)
{
    int32_t reading = div_round((int64_t)pp_mon_pin_microvolts(code) * VIN_RATIO, 1000);
    return reading - offset_mv;
}

pp_mon_rail5_t pp_mon_rail5(uint16_t code)
{
    uint32_t pin_uv = pp_mon_pin_microvolts(code);
    pp_mon_rail5_t rail;
    if (pin_uv > RAIL5_SOURCE_LIMIT_UV) {
        rail.input = PP_SUPPLY_USB_C;
        rail.millivolts = unscale_mv(pin_uv, SCALE_RAIL5_USB_C);
        rail.consistent = true;
    } else {
        rail.input = PP_SUPPLY_MODULE;
        rail.millivolts = unscale_mv(pin_uv, SCALE_RAIL5_MODULE);
        rail.consistent = rail.millivolts <= RAIL5_INCONSISTENT_MV;
    }
    return rail;
}

int32_t pp_mon_cc_mv(uint16_t code)
{
    return div_round((int64_t)pp_mon_pin_microvolts(code), 1000);
}

int32_t pp_mon_temp_decidegrees(uint16_t code)
{
    /* 10 mV per degree: one millivolt is a tenth of a degree. */
    return div_round((int64_t)pp_mon_pin_microvolts(code) - TEMP_ZERO_UV, 1000);
}

int32_t pp_mon_p12_mv(uint16_t code)
{
    return unscale_mv(pp_mon_pin_microvolts(code), SCALE_P12);
}

int32_t pp_mon_n4_mv(uint16_t code)
{
    return div_round((int64_t)pp_mon_pin_microvolts(code) - N4_OFFSET_UV, N4_SCALE_PER_MILLE);
}

static bool code_is_stuck(uint16_t code)
{
    return (code == 0u) || (code >= PP_MON_CODE_MAX);
}

uint32_t pp_mon_rails_test(const pp_mon_scan_t *scan, bool pwr_good_valid, int32_t rail5_average_mv)
{
    uint32_t reasons = pwr_good_valid ? 0u : (uint32_t)PP_RAILS_PWR_GOOD;
    if ((scan == NULL) || !scan->answered) {
        /* Without readings nothing else can be judged. */
        return reasons | (uint32_t)PP_RAILS_NO_ANSWER;
    }

    int32_t p12 = pp_mon_p12_mv(scan->code[PP_MON_CH_P12]);
    if ((p12 < P12_MIN_MV) || (p12 > P12_MAX_MV)) {
        reasons |= (uint32_t)PP_RAILS_P12;
    }

    int32_t n4 = pp_mon_n4_mv(scan->code[PP_MON_CH_N4]);
    if ((n4 < N4_MIN_MV) || (n4 > N4_MAX_MV)) {
        reasons |= (uint32_t)PP_RAILS_N4;
    }

    if (rail5_average_mv >= RAIL5_LIMIT_MV) {
        reasons |= (uint32_t)PP_RAILS_RAIL5_HIGH;
    }
    if ((rail5_average_mv < RAIL5_MIN_MV) ||
        !pp_mon_rail5(scan->code[PP_MON_CH_RAIL5]).consistent) {
        reasons |= (uint32_t)PP_RAILS_RAIL5_LOW;
    }

    uint32_t temp_uv = pp_mon_pin_microvolts(scan->code[PP_MON_CH_TEMP]);
    if ((temp_uv < TEMP_MIN_UV) || (temp_uv > TEMP_MAX_UV)) {
        reasons |= (uint32_t)PP_RAILS_TEMP;
    }

    if (code_is_stuck(scan->code[PP_MON_CH_RAIL5]) || code_is_stuck(scan->code[PP_MON_CH_P12]) ||
        code_is_stuck(scan->code[PP_MON_CH_N4])) {
        reasons |= (uint32_t)PP_RAILS_STUCK;
    }
    return reasons;
}

bool pp_mon_rails_flag(uint32_t reasons)
{
    return (reasons & ~(uint32_t)PP_RAILS_RAIL5_LOW) != 0u;
}
