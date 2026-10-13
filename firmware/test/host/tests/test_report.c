/*
 * The records of section 7.4 of the specification against the shared test
 * vectors. A vector holds the bytes of a record, its numeric fields in the
 * order of the layout and its text.
 */
#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "pp_proto_vectors.h"
#include "proto/pp_report.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_report_sizes_of_the_definition(void)
{
    TEST_ASSERT_EQUAL_UINT(48u, PP_INFO_SIZE);
    TEST_ASSERT_EQUAL_UINT(25u, PP_STATUS_SIZE);
    TEST_ASSERT_EQUAL_UINT(4u, PP_SUPPLY_SIZE);
    TEST_ASSERT_EQUAL_UINT(16u, PP_ZERO_SIZE);
    TEST_ASSERT_GREATER_THAN_size_t(0u, PP_INFO_VECTOR_COUNT);
    TEST_ASSERT_GREATER_THAN_size_t(0u, PP_STATUS_VECTOR_COUNT);
    TEST_ASSERT_GREATER_THAN_size_t(0u, PP_SUPPLY_VECTOR_COUNT);
    TEST_ASSERT_GREATER_THAN_size_t(0u, PP_ZERO_VECTOR_COUNT);
}

static void test_info_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_INFO_VECTOR_COUNT; i++) {
        const pp_record_vector_t *vector = &pp_info_vectors[i];
        const int64_t *v = vector->values;
        pp_info_t info;
        uint8_t out[PP_INFO_SIZE];
        size_t len = 0u;

        TEST_ASSERT_EQUAL_size_t(13u, vector->value_count);
        TEST_ASSERT_EQUAL_MESSAGE(PP_OK, pp_info_decode(vector->data, vector->data_len, &info),
                                  vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[0], info.protocol_version, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[1], info.hardware_revision, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[2], info.firmware_major, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[3], info.firmware_minor, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[4], info.firmware_patch, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[5], info.cal_state, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[6], info.settle_samples, vector->name);
        TEST_ASSERT_EQUAL_HEX64_MESSAGE((uint64_t)v[7], info.chip_id, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[8], info.carrier_revision, vector->name);
        for (size_t range = 0u; range < PP_RANGE_COUNT; range++) {
            TEST_ASSERT_EQUAL_INT64_MESSAGE(v[9u + range], info.zero[range], vector->name);
        }
        TEST_ASSERT_EQUAL_STRING_MESSAGE(vector->text, info.carrier_serial, vector->name);

        TEST_ASSERT_EQUAL(PP_OK, pp_info_encode(&info, out, sizeof(out), &len));
        TEST_ASSERT_EQUAL_size_t(vector->data_len, len);
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(vector->data, out, len, vector->name);
    }
}

static void test_status_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_STATUS_VECTOR_COUNT; i++) {
        const pp_record_vector_t *vector = &pp_status_vectors[i];
        const int64_t *v = vector->values;
        pp_device_status_t status;
        uint8_t out[PP_STATUS_SIZE];
        size_t len = 0u;

        TEST_ASSERT_EQUAL_size_t(15u, vector->value_count);
        TEST_ASSERT_EQUAL_MESSAGE(
            PP_OK, pp_device_status_decode(vector->data, vector->data_len, &status), vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[0], status.state, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[1], status.faults, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[2], status.dropped_blocks, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[3], status.mode, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[4], status.range_lock, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[5], status.setpoint_mv, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[6], status.vout_mv, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[7], status.vin_mv, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[8], status.rail5_mv, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[9], status.temperature_dc, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[10], status.supply, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[11], status.cc_class, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[12], status.budget_ma, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[13], status.trip_count, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[14], status.notices, vector->name);

        TEST_ASSERT_EQUAL(PP_OK, pp_device_status_encode(&status, out, sizeof(out), &len));
        TEST_ASSERT_EQUAL_size_t(vector->data_len, len);
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(vector->data, out, len, vector->name);
    }
}

static void test_supply_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_SUPPLY_VECTOR_COUNT; i++) {
        const pp_record_vector_t *vector = &pp_supply_vectors[i];
        const int64_t *v = vector->values;
        pp_supply_report_t report;
        uint8_t out[PP_SUPPLY_SIZE];
        size_t len = 0u;

        TEST_ASSERT_EQUAL_size_t(3u, vector->value_count);
        TEST_ASSERT_EQUAL_MESSAGE(
            PP_OK, pp_supply_report_decode(vector->data, vector->data_len, &report), vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[0], report.supply, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[1], report.cc_class, vector->name);
        TEST_ASSERT_EQUAL_INT64_MESSAGE(v[2], report.budget_ma, vector->name);

        TEST_ASSERT_EQUAL(PP_OK, pp_supply_report_encode(&report, out, sizeof(out), &len));
        TEST_ASSERT_EQUAL_size_t(vector->data_len, len);
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(vector->data, out, len, vector->name);
    }
}

static void test_zero_matches_the_shared_vectors(void)
{
    for (size_t i = 0u; i < PP_ZERO_VECTOR_COUNT; i++) {
        const pp_record_vector_t *vector = &pp_zero_vectors[i];
        pp_zero_report_t report;
        uint8_t out[PP_ZERO_SIZE];
        size_t len = 0u;

        TEST_ASSERT_EQUAL_size_t(PP_RANGE_COUNT, vector->value_count);
        TEST_ASSERT_EQUAL_MESSAGE(
            PP_OK, pp_zero_report_decode(vector->data, vector->data_len, &report), vector->name);
        for (size_t range = 0u; range < PP_RANGE_COUNT; range++) {
            TEST_ASSERT_EQUAL_INT64_MESSAGE(vector->values[range], report.zero[range],
                                            vector->name);
        }

        TEST_ASSERT_EQUAL(PP_OK, pp_zero_report_encode(&report, out, sizeof(out), &len));
        TEST_ASSERT_EQUAL_size_t(vector->data_len, len);
        TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(vector->data, out, len, vector->name);
    }
}

static void test_report_encoders_check_their_arguments(void)
{
    const pp_info_t info = {0};
    const pp_device_status_t status = {0};
    const pp_supply_report_t supply = {0};
    const pp_zero_report_t zero = {0};
    uint8_t out[PP_INFO_SIZE];

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_info_encode(NULL, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_info_encode(&info, NULL, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_ERR_NO_SPACE, pp_info_encode(&info, out, PP_INFO_SIZE - 1u, NULL));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_device_status_encode(NULL, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_device_status_encode(&status, NULL, 64u, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_NO_SPACE,
                      pp_device_status_encode(&status, out, PP_STATUS_SIZE - 1u, NULL));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_supply_report_encode(NULL, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_ERR_NO_SPACE,
                      pp_supply_report_encode(&supply, out, PP_SUPPLY_SIZE - 1u, NULL));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_zero_report_encode(NULL, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_ERR_NO_SPACE, pp_zero_report_encode(&zero, out, PP_ZERO_SIZE - 1u, NULL));

    /* The length is optional. */
    TEST_ASSERT_EQUAL(PP_OK, pp_info_encode(&info, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_OK, pp_device_status_encode(&status, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_OK, pp_supply_report_encode(&supply, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL(PP_OK, pp_zero_report_encode(&zero, out, sizeof(out), NULL));
}

static void test_report_decoders_check_their_arguments(void)
{
    static const uint8_t data[PP_INFO_SIZE] = {0};
    pp_info_t info;
    pp_device_status_t status;
    pp_supply_report_t supply;
    pp_zero_report_t zero;

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_info_decode(NULL, PP_INFO_SIZE, &info));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_info_decode(data, PP_INFO_SIZE, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_MALFORMED, pp_info_decode(data, PP_INFO_SIZE - 1u, &info));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_device_status_decode(NULL, PP_STATUS_SIZE, &status));
    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_device_status_decode(data, PP_STATUS_SIZE, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_MALFORMED,
                      pp_device_status_decode(data, PP_STATUS_SIZE - 1u, &status));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_supply_report_decode(data, PP_SUPPLY_SIZE, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_MALFORMED,
                      pp_supply_report_decode(data, PP_SUPPLY_SIZE - 1u, &supply));

    TEST_ASSERT_EQUAL(PP_ERR_INVALID_ARG, pp_zero_report_decode(data, PP_ZERO_SIZE, NULL));
    TEST_ASSERT_EQUAL(PP_ERR_MALFORMED, pp_zero_report_decode(data, PP_ZERO_SIZE - 1u, &zero));
}

/* Section 7.4: a receiver ignores bytes after the fields it knows. */
static void test_report_decoders_ignore_appended_bytes(void)
{
    const pp_record_vector_t *vector = &pp_status_vectors[0];
    uint8_t longer[PP_STATUS_SIZE + 6u];
    pp_device_status_t plain;
    pp_device_status_t extended;

    /* The comparison below reads the padding of the structures too. */
    memset(&plain, 0, sizeof(plain));
    memset(&extended, 0, sizeof(extended));
    memcpy(longer, vector->data, PP_STATUS_SIZE);
    memset(longer + PP_STATUS_SIZE, 0xEE, 6u);

    TEST_ASSERT_EQUAL(PP_OK, pp_device_status_decode(vector->data, PP_STATUS_SIZE, &plain));
    TEST_ASSERT_EQUAL(PP_OK, pp_device_status_decode(longer, sizeof(longer), &extended));
    TEST_ASSERT_EQUAL_MEMORY(&plain, &extended, sizeof(plain));
}

static void test_info_text_is_padded_and_cut_to_its_field(void)
{
    pp_info_t info = {0};
    pp_info_t back;
    uint8_t out[PP_INFO_SIZE];
    const uint8_t *serial = out + PP_INFO_OFFSET_CARRIER_SERIAL;

    /* A short text is followed by zeros, whatever the buffer held. */
    memset(out, 0xFF, sizeof(out));
    memcpy(info.carrier_serial, "AB", 3u);
    TEST_ASSERT_EQUAL(PP_OK, pp_info_encode(&info, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL_HEX8('A', serial[0]);
    TEST_ASSERT_EQUAL_HEX8('B', serial[1]);
    for (size_t i = 2u; i < PP_SERIAL_SIZE; i++) {
        TEST_ASSERT_EQUAL_HEX8(0u, serial[i]);
    }

    /* A text of the full length has no terminator on the wire. */
    memset(info.carrier_serial, 'Z', PP_SERIAL_SIZE);
    info.carrier_serial[PP_SERIAL_SIZE] = '\0';
    TEST_ASSERT_EQUAL(PP_OK, pp_info_encode(&info, out, sizeof(out), NULL));
    TEST_ASSERT_EQUAL_HEX8('Z', serial[PP_SERIAL_SIZE - 1u]);
    TEST_ASSERT_EQUAL(PP_OK, pp_info_decode(out, sizeof(out), &back));
    TEST_ASSERT_EQUAL_size_t(PP_SERIAL_SIZE, strlen(back.carrier_serial));
    TEST_ASSERT_EQUAL_STRING(info.carrier_serial, back.carrier_serial);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_report_sizes_of_the_definition);
    RUN_TEST(test_info_matches_the_shared_vectors);
    RUN_TEST(test_status_matches_the_shared_vectors);
    RUN_TEST(test_supply_matches_the_shared_vectors);
    RUN_TEST(test_zero_matches_the_shared_vectors);
    RUN_TEST(test_report_encoders_check_their_arguments);
    RUN_TEST(test_report_decoders_check_their_arguments);
    RUN_TEST(test_report_decoders_ignore_appended_bytes);
    RUN_TEST(test_info_text_is_padded_and_cut_to_its_field);
    return UNITY_END();
}
