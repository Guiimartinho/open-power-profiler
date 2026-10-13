#include "unity.h"

#include "base/pp_bytes.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_bytes_put_u16le_writes_low_byte_first(void)
{
    uint8_t buffer[4] = {0xEEu, 0xEEu, 0xEEu, 0xEEu};

    pp_put_u16le(buffer + 1, 0xA55Au);

    const uint8_t expected[4] = {0xEEu, 0x5Au, 0xA5u, 0xEEu};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_bytes_put_u32le_writes_low_byte_first(void)
{
    uint8_t buffer[6] = {0xEEu, 0xEEu, 0xEEu, 0xEEu, 0xEEu, 0xEEu};

    pp_put_u32le(buffer + 1, 0x12345678u);

    const uint8_t expected[6] = {0xEEu, 0x78u, 0x56u, 0x34u, 0x12u, 0xEEu};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_bytes_get_u16le_reads_low_byte_first(void)
{
    const uint8_t buffer[2] = {0x34u, 0x12u};

    TEST_ASSERT_EQUAL_HEX16(0x1234u, pp_get_u16le(buffer));
}

static void test_bytes_get_u32le_reads_low_byte_first(void)
{
    const uint8_t buffer[4] = {0x78u, 0x56u, 0x34u, 0x12u};

    TEST_ASSERT_EQUAL_HEX32(0x12345678u, pp_get_u32le(buffer));
}

static void test_bytes_round_trip_at_the_limits(void)
{
    const uint16_t values16[] = {0x0000u, 0x0001u, 0x00FFu, 0x0100u, 0x8000u, 0xFFFFu};
    const uint32_t values32[] = {0x00000000u, 0x00000001u, 0x000000FFu, 0x0000FF00u,
                                 0x00FF0000u, 0xFF000000u, 0x80000000u, 0xFFFFFFFFu};
    uint8_t buffer[4];

    for (size_t i = 0u; i < (sizeof(values16) / sizeof(values16[0])); i++) {
        pp_put_u16le(buffer, values16[i]);
        TEST_ASSERT_EQUAL_HEX16(values16[i], pp_get_u16le(buffer));
    }
    for (size_t i = 0u; i < (sizeof(values32) / sizeof(values32[0])); i++) {
        pp_put_u32le(buffer, values32[i]);
        TEST_ASSERT_EQUAL_HEX32(values32[i], pp_get_u32le(buffer));
    }
}

static void test_bytes_u64le_writes_and_reads_low_byte_first(void)
{
    const uint8_t expected[8] = {0xEFu, 0xCDu, 0xABu, 0x89u, 0x67u, 0x45u, 0x23u, 0x01u};
    uint8_t buffer[8] = {0};

    pp_put_u64le(buffer, UINT64_C(0x0123456789ABCDEF));

    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buffer, 8u);
    TEST_ASSERT_EQUAL_HEX64(UINT64_C(0x0123456789ABCDEF), pp_get_u64le(expected));
}

static void test_bytes_u64le_round_trip_at_the_limits(void)
{
    const uint64_t values[] = {UINT64_C(0),
                               UINT64_C(1),
                               UINT64_C(0x00000000FFFFFFFF),
                               UINT64_C(0x0000000100000000),
                               UINT64_C(0x8000000000000000),
                               UINT64_C(0xFFFFFFFFFFFFFFFF)};
    uint8_t buffer[8];

    for (size_t i = 0u; i < (sizeof(values) / sizeof(values[0])); i++) {
        pp_put_u64le(buffer, values[i]);
        TEST_ASSERT_EQUAL_HEX64(values[i], pp_get_u64le(buffer));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bytes_put_u16le_writes_low_byte_first);
    RUN_TEST(test_bytes_put_u32le_writes_low_byte_first);
    RUN_TEST(test_bytes_get_u16le_reads_low_byte_first);
    RUN_TEST(test_bytes_get_u32le_reads_low_byte_first);
    RUN_TEST(test_bytes_round_trip_at_the_limits);
    RUN_TEST(test_bytes_u64le_writes_and_reads_low_byte_first);
    RUN_TEST(test_bytes_u64le_round_trip_at_the_limits);
    return UNITY_END();
}
