/*
 * Records with a fixed layout (section 7.4 of the specification): the data
 * of GET_INFO and GET_STATUS, of the event "supply changed" and of the zero
 * offsets that CAL_ZERO and the notice "zero taken" carry.
 *
 * The layouts are lists of fields in protocol/definition.toml. The generated
 * header gives the size of each record and the offset of each field, and the
 * shared test vectors hold records that this codec and the one of the host
 * must reproduce byte for byte.
 *
 * A decoder accepts a record that is longer than the layout it knows and
 * ignores the rest, so that fields can be appended.
 */
#ifndef PP_REPORT_H
#define PP_REPORT_H

#include <stddef.h>
#include <stdint.h>

#include "base/pp_status.h"
#include "proto/pp_proto_defs.h"

/* Data of GET_INFO. */
typedef struct {
    uint8_t protocol_version;
    uint8_t hardware_revision;
    uint8_t firmware_major;
    uint8_t firmware_minor;
    uint8_t firmware_patch;
    uint8_t cal_state;                        /* a pp_cal_state_t */
    uint8_t settle_samples;                   /* length of the settling window (F-35) */
    uint64_t chip_id;                         /* identifier of the controller (D-82) */
    char carrier_serial[PP_SERIAL_SIZE + 1u]; /* text, always terminated */
    uint8_t carrier_revision;
    int32_t zero[PP_RANGE_COUNT]; /* zero offsets, PP_ZERO_SCALE units per code */
} pp_info_t;

/* Data of GET_STATUS. */
typedef struct {
    uint8_t state;   /* a pp_device_state_t */
    uint16_t faults; /* PP_FAULT_ bits */
    uint32_t dropped_blocks;
    uint8_t mode;       /* a pp_mode_t */
    uint8_t range_lock; /* 0 to 3, or PP_RANGE_AUTO */
    uint16_t setpoint_mv;
    uint16_t vout_mv;
    int16_t vin_mv;
    uint16_t rail5_mv;
    int16_t temperature_dc; /* tenths of a degree Celsius */
    uint8_t supply;         /* a pp_supply_t */
    uint8_t cc_class;       /* a pp_cc_class_t */
    uint16_t budget_ma;
    uint8_t trip_count; /* consecutive trips (F-19) */
    uint8_t notices;    /* PP_STATUS_NOTICE_ bits */
} pp_device_status_t;

/* Data of the event "supply changed" (F-14). */
typedef struct {
    uint8_t supply;   /* a pp_supply_t */
    uint8_t cc_class; /* a pp_cc_class_t */
    uint16_t budget_ma;
} pp_supply_report_t;

/* Zero offsets of the four ranges, PP_ZERO_SCALE units per code. */
typedef struct {
    int32_t zero[PP_RANGE_COUNT];
} pp_zero_report_t;

/*
 * The encoders write exactly the size of their record and report it through
 * `len`, which may be NULL. They return PP_ERR_INVALID_ARG for a NULL record
 * or buffer and PP_ERR_NO_SPACE when the buffer is too small.
 *
 * The decoders return PP_ERR_INVALID_ARG for a NULL argument and
 * PP_ERR_MALFORMED for data shorter than the record.
 */
pp_status_t pp_info_encode(const pp_info_t *info, uint8_t *out, size_t out_size, size_t *len);
pp_status_t pp_info_decode(const uint8_t *data, size_t data_len, pp_info_t *info);

pp_status_t pp_device_status_encode(const pp_device_status_t *status, uint8_t *out, size_t out_size,
                                    size_t *len);
pp_status_t pp_device_status_decode(const uint8_t *data, size_t data_len,
                                    pp_device_status_t *status);

pp_status_t pp_supply_report_encode(const pp_supply_report_t *report, uint8_t *out, size_t out_size,
                                    size_t *len);
pp_status_t pp_supply_report_decode(const uint8_t *data, size_t data_len,
                                    pp_supply_report_t *report);

pp_status_t pp_zero_report_encode(const pp_zero_report_t *report, uint8_t *out, size_t out_size,
                                  size_t *len);
pp_status_t pp_zero_report_decode(const uint8_t *data, size_t data_len, pp_zero_report_t *report);

#endif /* PP_REPORT_H */
