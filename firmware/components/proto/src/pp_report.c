#include "proto/pp_report.h"

#include <string.h>

#include "base/pp_bytes.h"

/* The four zero offsets follow each other, four bytes apart, in both layouts. */
#define ZERO_STEP 4u
_Static_assert(PP_RANGE_COUNT == 4u, "the layouts hold four zero offsets");
_Static_assert(PP_INFO_OFFSET_ZERO_R3 == (PP_INFO_OFFSET_ZERO_R0 + (3u * ZERO_STEP)),
               "zero offsets of the info record are not consecutive");
_Static_assert(PP_ZERO_OFFSET_ZERO_R3 == (PP_ZERO_OFFSET_ZERO_R0 + (3u * ZERO_STEP)),
               "zero offsets of the zero record are not consecutive");
_Static_assert(PP_INFO_SIZE == (PP_INFO_OFFSET_ZERO_R3 + ZERO_STEP), "info record has a tail");

/* Arguments of an encoder: PP_OK when `size` bytes can be written to `out`. */
static pp_status_t check_encode(const void *record, const uint8_t *out, size_t out_size,
                                size_t size)
{
    if ((record == NULL) || (out == NULL)) {
        return PP_ERR_INVALID_ARG;
    }
    return (out_size < size) ? PP_ERR_NO_SPACE : PP_OK;
}

/* Arguments of a decoder: PP_OK when `data` holds a record of `size` bytes. */
static pp_status_t check_decode(const uint8_t *data, size_t data_len, const void *record,
                                size_t size)
{
    if ((data == NULL) || (record == NULL)) {
        return PP_ERR_INVALID_ARG;
    }
    return (data_len < size) ? PP_ERR_MALFORMED : PP_OK;
}

static void set_len(size_t *len, size_t value)
{
    if (len != NULL) {
        *len = value;
    }
}

static void put_zero(uint8_t *dst, const int32_t *zero)
{
    for (size_t i = 0u; i < PP_RANGE_COUNT; i++) {
        pp_put_u32le(dst + (i * ZERO_STEP), (uint32_t)zero[i]);
    }
}

static void get_zero(const uint8_t *src, int32_t *zero)
{
    for (size_t i = 0u; i < PP_RANGE_COUNT; i++) {
        zero[i] = (int32_t)pp_get_u32le(src + (i * ZERO_STEP));
    }
}

pp_status_t pp_info_encode(const pp_info_t *info, uint8_t *out, size_t out_size, size_t *len)
{
    pp_status_t status = check_encode(info, out, out_size, PP_INFO_SIZE);
    if (status != PP_OK) {
        return status;
    }
    out[PP_INFO_OFFSET_PROTOCOL_VERSION] = info->protocol_version;
    out[PP_INFO_OFFSET_HARDWARE_REVISION] = info->hardware_revision;
    out[PP_INFO_OFFSET_FIRMWARE_MAJOR] = info->firmware_major;
    out[PP_INFO_OFFSET_FIRMWARE_MINOR] = info->firmware_minor;
    out[PP_INFO_OFFSET_FIRMWARE_PATCH] = info->firmware_patch;
    out[PP_INFO_OFFSET_CAL_STATE] = info->cal_state;
    out[PP_INFO_OFFSET_SETTLE_SAMPLES] = info->settle_samples;
    pp_put_u64le(out + PP_INFO_OFFSET_CHIP_ID, info->chip_id);

    /* The text is padded with zeros and loses what does not fit. */
    uint8_t *serial = out + PP_INFO_OFFSET_CARRIER_SERIAL;
    size_t used = 0u;
    while ((used < PP_SERIAL_SIZE) && (info->carrier_serial[used] != '\0')) {
        serial[used] = (uint8_t)info->carrier_serial[used];
        used++;
    }
    memset(serial + used, 0, PP_SERIAL_SIZE - used);

    out[PP_INFO_OFFSET_CARRIER_REVISION] = info->carrier_revision;
    put_zero(out + PP_INFO_OFFSET_ZERO_R0, info->zero);
    set_len(len, PP_INFO_SIZE);
    return PP_OK;
}

pp_status_t pp_info_decode(const uint8_t *data, size_t data_len, pp_info_t *info)
{
    pp_status_t status = check_decode(data, data_len, info, PP_INFO_SIZE);
    if (status != PP_OK) {
        return status;
    }
    info->protocol_version = data[PP_INFO_OFFSET_PROTOCOL_VERSION];
    info->hardware_revision = data[PP_INFO_OFFSET_HARDWARE_REVISION];
    info->firmware_major = data[PP_INFO_OFFSET_FIRMWARE_MAJOR];
    info->firmware_minor = data[PP_INFO_OFFSET_FIRMWARE_MINOR];
    info->firmware_patch = data[PP_INFO_OFFSET_FIRMWARE_PATCH];
    info->cal_state = data[PP_INFO_OFFSET_CAL_STATE];
    info->settle_samples = data[PP_INFO_OFFSET_SETTLE_SAMPLES];
    info->chip_id = pp_get_u64le(data + PP_INFO_OFFSET_CHIP_ID);
    memcpy(info->carrier_serial, data + PP_INFO_OFFSET_CARRIER_SERIAL, PP_SERIAL_SIZE);
    info->carrier_serial[PP_SERIAL_SIZE] = '\0';
    info->carrier_revision = data[PP_INFO_OFFSET_CARRIER_REVISION];
    get_zero(data + PP_INFO_OFFSET_ZERO_R0, info->zero);
    return PP_OK;
}

pp_status_t pp_device_status_encode(const pp_device_status_t *status, uint8_t *out, size_t out_size,
                                    size_t *len)
{
    pp_status_t result = check_encode(status, out, out_size, PP_STATUS_SIZE);
    if (result != PP_OK) {
        return result;
    }
    out[PP_STATUS_OFFSET_STATE] = status->state;
    pp_put_u16le(out + PP_STATUS_OFFSET_FAULTS, status->faults);
    pp_put_u32le(out + PP_STATUS_OFFSET_DROPPED_BLOCKS, status->dropped_blocks);
    out[PP_STATUS_OFFSET_MODE] = status->mode;
    out[PP_STATUS_OFFSET_RANGE_LOCK] = status->range_lock;
    pp_put_u16le(out + PP_STATUS_OFFSET_SETPOINT_MV, status->setpoint_mv);
    pp_put_u16le(out + PP_STATUS_OFFSET_VOUT_MV, status->vout_mv);
    pp_put_u16le(out + PP_STATUS_OFFSET_VIN_MV, (uint16_t)status->vin_mv);
    pp_put_u16le(out + PP_STATUS_OFFSET_RAIL5_MV, status->rail5_mv);
    pp_put_u16le(out + PP_STATUS_OFFSET_TEMPERATURE_DC, (uint16_t)status->temperature_dc);
    out[PP_STATUS_OFFSET_SUPPLY] = status->supply;
    out[PP_STATUS_OFFSET_CC_CLASS] = status->cc_class;
    pp_put_u16le(out + PP_STATUS_OFFSET_BUDGET_MA, status->budget_ma);
    out[PP_STATUS_OFFSET_TRIP_COUNT] = status->trip_count;
    out[PP_STATUS_OFFSET_NOTICES] = status->notices;
    set_len(len, PP_STATUS_SIZE);
    return PP_OK;
}

pp_status_t pp_device_status_decode(const uint8_t *data, size_t data_len,
                                    pp_device_status_t *status)
{
    pp_status_t result = check_decode(data, data_len, status, PP_STATUS_SIZE);
    if (result != PP_OK) {
        return result;
    }
    status->state = data[PP_STATUS_OFFSET_STATE];
    status->faults = pp_get_u16le(data + PP_STATUS_OFFSET_FAULTS);
    status->dropped_blocks = pp_get_u32le(data + PP_STATUS_OFFSET_DROPPED_BLOCKS);
    status->mode = data[PP_STATUS_OFFSET_MODE];
    status->range_lock = data[PP_STATUS_OFFSET_RANGE_LOCK];
    status->setpoint_mv = pp_get_u16le(data + PP_STATUS_OFFSET_SETPOINT_MV);
    status->vout_mv = pp_get_u16le(data + PP_STATUS_OFFSET_VOUT_MV);
    status->vin_mv = (int16_t)pp_get_u16le(data + PP_STATUS_OFFSET_VIN_MV);
    status->rail5_mv = pp_get_u16le(data + PP_STATUS_OFFSET_RAIL5_MV);
    status->temperature_dc = (int16_t)pp_get_u16le(data + PP_STATUS_OFFSET_TEMPERATURE_DC);
    status->supply = data[PP_STATUS_OFFSET_SUPPLY];
    status->cc_class = data[PP_STATUS_OFFSET_CC_CLASS];
    status->budget_ma = pp_get_u16le(data + PP_STATUS_OFFSET_BUDGET_MA);
    status->trip_count = data[PP_STATUS_OFFSET_TRIP_COUNT];
    status->notices = data[PP_STATUS_OFFSET_NOTICES];
    return PP_OK;
}

pp_status_t pp_supply_report_encode(const pp_supply_report_t *report, uint8_t *out, size_t out_size,
                                    size_t *len)
{
    pp_status_t status = check_encode(report, out, out_size, PP_SUPPLY_SIZE);
    if (status != PP_OK) {
        return status;
    }
    out[PP_SUPPLY_OFFSET_SUPPLY] = report->supply;
    out[PP_SUPPLY_OFFSET_CC_CLASS] = report->cc_class;
    pp_put_u16le(out + PP_SUPPLY_OFFSET_BUDGET_MA, report->budget_ma);
    set_len(len, PP_SUPPLY_SIZE);
    return PP_OK;
}

pp_status_t pp_supply_report_decode(const uint8_t *data, size_t data_len,
                                    pp_supply_report_t *report)
{
    pp_status_t status = check_decode(data, data_len, report, PP_SUPPLY_SIZE);
    if (status != PP_OK) {
        return status;
    }
    report->supply = data[PP_SUPPLY_OFFSET_SUPPLY];
    report->cc_class = data[PP_SUPPLY_OFFSET_CC_CLASS];
    report->budget_ma = pp_get_u16le(data + PP_SUPPLY_OFFSET_BUDGET_MA);
    return PP_OK;
}

pp_status_t pp_zero_report_encode(const pp_zero_report_t *report, uint8_t *out, size_t out_size,
                                  size_t *len)
{
    pp_status_t status = check_encode(report, out, out_size, PP_ZERO_SIZE);
    if (status != PP_OK) {
        return status;
    }
    put_zero(out + PP_ZERO_OFFSET_ZERO_R0, report->zero);
    set_len(len, PP_ZERO_SIZE);
    return PP_OK;
}

pp_status_t pp_zero_report_decode(const uint8_t *data, size_t data_len, pp_zero_report_t *report)
{
    pp_status_t status = check_decode(data, data_len, report, PP_ZERO_SIZE);
    if (status != PP_OK) {
        return status;
    }
    get_zero(data + PP_ZERO_OFFSET_ZERO_R0, report->zero);
    return PP_OK;
}
