/*
 * Set-point DAC of the source (sections 4.2 and 6.6 of the specification,
 * rules F-6 and F-30).
 *
 * The DAC is a 12-bit part on the slow SPI bus. One 16-bit frame carries a
 * command nibble and the code. Rule F-6 fixes the nibble: reference input
 * buffered, gain 2, output active. Every frame goes through pp_dac_frame(),
 * so no other nibble can reach the bus.
 *
 * Before calibration the output of the source is 10 mV plus 1.2817 mV per
 * code (F-30). A set-point from the host is limited to 0.80 V to 5.00 V.
 */
#ifndef PP_DAC_H
#define PP_DAC_H

#include <stdint.h>

#include "base/pp_status.h"

/* Largest code of the 12-bit DAC. */
#define PP_DAC_CODE_MAX 4095u

/* Upper four bits of every frame: write, buffered reference, gain 2, active. */
#define PP_DAC_COMMAND 0x5000u

/* Limits of SET_VOLTAGE in millivolts (F-30). */
#define PP_SETPOINT_MIN_MV 800u
#define PP_SETPOINT_MAX_MV 5000u

/* Nominal scale of the source: output = offset + code * step. */
#define PP_DAC_NOMINAL_OFFSET_UV 10000u /* 10 mV, in microvolts */
#define PP_DAC_NOMINAL_STEP_NV 1281700u /* 1.2817 mV, in nanovolts */

/*
 * The SPI frame for a code. A code above PP_DAC_CODE_MAX is sent as
 * PP_DAC_CODE_MAX: the command nibble never changes.
 */
uint16_t pp_dac_frame(uint16_t code);

/* Nominal output of the source for a code, in microvolts. */
uint32_t pp_dac_nominal_microvolts(uint16_t code);

/*
 * Nominal code for a set-point in millivolts, rounded to the nearest code.
 * Returns PP_ERR_INVALID_ARG for a NULL result or a set-point outside
 * PP_SETPOINT_MIN_MV to PP_SETPOINT_MAX_MV, and writes nothing then.
 */
pp_status_t pp_dac_nominal_code(uint16_t millivolts, uint16_t *code);

#endif /* PP_DAC_H */
