#include "smu/pp_dac.h"

#include <stddef.h>

uint16_t pp_dac_frame(uint16_t code)
{
    uint16_t limited = (code > PP_DAC_CODE_MAX) ? (uint16_t)PP_DAC_CODE_MAX : code;
    return (uint16_t)(PP_DAC_COMMAND | limited);
}

uint32_t pp_dac_nominal_microvolts(uint16_t code)
{
    uint64_t limited = (code > PP_DAC_CODE_MAX) ? PP_DAC_CODE_MAX : code;
    uint64_t nanovolts = limited * PP_DAC_NOMINAL_STEP_NV;
    return (uint32_t)(PP_DAC_NOMINAL_OFFSET_UV + ((nanovolts + 500u) / 1000u));
}

pp_status_t pp_dac_nominal_code(uint16_t millivolts, uint16_t *code)
{
    if ((code == NULL) || (millivolts < PP_SETPOINT_MIN_MV) || (millivolts > PP_SETPOINT_MAX_MV)) {
        return PP_ERR_INVALID_ARG;
    }
    /* The lowest set-point is far above the offset, so this cannot go negative. */
    uint64_t nanovolts = (((uint64_t)millivolts * 1000u) - PP_DAC_NOMINAL_OFFSET_UV) * 1000u;
    *code = (uint16_t)((nanovolts + (PP_DAC_NOMINAL_STEP_NV / 2u)) / PP_DAC_NOMINAL_STEP_NV);
    return PP_OK;
}
