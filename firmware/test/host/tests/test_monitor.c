/*
 * The monitor channels against section 4.2 of the specification and rules
 * F-11 and F-12 of section 6.6. The expected values are calculated here in
 * floating point from the factors of the specification, independently of the
 * integer arithmetic under test.
 */
#include "unity.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "monitor/pp_monitor.h"

static pp_mon_scan_t scan;

/* Voltage at the converter pin for a code, in volts. */
static double pin_volts(unsigned code)
{
    return ((double)code * 2.5) / 4096.0;
}

/* The code the converter gives for a voltage at its pin. */
static uint16_t code_for(double volts)
{
    return (uint16_t)lround((volts * 4096.0) / 2.5);
}

/* A scan with every rail at its nominal value, on the USB-C input at 25 degrees. */
static void nominal_scan(void)
{
    scan.answered = true;
    scan.code[PP_MON_CH_VOUT] = code_for(3.3 * 0.4545);
    scan.code[PP_MON_CH_VIN] = code_for(3.3 / 44.0);
    scan.code[PP_MON_CH_RAIL5] = code_for(5.0 * 0.4545);
    scan.code[PP_MON_CH_CC1] = code_for(0.92);
    scan.code[PP_MON_CH_CC2] = code_for(0.0);
    scan.code[PP_MON_CH_TEMP] = code_for(0.75);
    scan.code[PP_MON_CH_P12] = code_for(12.0 * 0.1754);
    scan.code[PP_MON_CH_N4] = code_for((-4.0 * 0.333) + 1.667);
}

void setUp(void)
{
    nominal_scan();
}

void tearDown(void)
{
}

static void test_monitor_pin_voltage_of_a_code(void)
{
    TEST_ASSERT_EQUAL_UINT32(0u, pp_mon_pin_microvolts(0u));
    TEST_ASSERT_EQUAL_UINT32(610u, pp_mon_pin_microvolts(1u));
    TEST_ASSERT_EQUAL_UINT32(1250000u, pp_mon_pin_microvolts(2048u));
    TEST_ASSERT_EQUAL_UINT32(2499390u, pp_mon_pin_microvolts(4095u));
    /* A code the converter cannot give is read as its full scale. */
    TEST_ASSERT_EQUAL_UINT32(2499390u, pp_mon_pin_microvolts(0xFFFFu));
}

static void test_monitor_scales_agree_with_the_specification_for_every_code(void)
{
    for (unsigned code = 0u; code <= PP_MON_CODE_MAX; code++) {
        double pin = pin_volts(code);
        uint16_t c = (uint16_t)code;
        TEST_ASSERT_INT32_WITHIN(1, lround((pin / 0.4545) * 1000.0), pp_mon_vout_mv(c));
        TEST_ASSERT_INT32_WITHIN(1, lround(pin * 44.0 * 1000.0), pp_mon_vin_mv(c, 0));
        TEST_ASSERT_INT32_WITHIN(1, lround(pin * 1000.0), pp_mon_cc_mv(c));
        TEST_ASSERT_INT32_WITHIN(1, lround((pin - 0.5) * 1000.0), pp_mon_temp_decidegrees(c));
        TEST_ASSERT_INT32_WITHIN(1, lround((pin / 0.1754) * 1000.0), pp_mon_p12_mv(c));
        TEST_ASSERT_INT32_WITHIN(1, lround(((pin - 1.667) / 0.333) * 1000.0), pp_mon_n4_mv(c));
    }
}

static void test_monitor_nominal_values(void)
{
    TEST_ASSERT_INT32_WITHIN(2, 3300, pp_mon_vout_mv(scan.code[PP_MON_CH_VOUT]));
    TEST_ASSERT_INT32_WITHIN(14, 3300, pp_mon_vin_mv(scan.code[PP_MON_CH_VIN], 0));
    TEST_ASSERT_INT32_WITHIN(1, 920, pp_mon_cc_mv(scan.code[PP_MON_CH_CC1]));
    TEST_ASSERT_INT32_WITHIN(1, 250, pp_mon_temp_decidegrees(scan.code[PP_MON_CH_TEMP]));
    TEST_ASSERT_INT32_WITHIN(4, 12000, pp_mon_p12_mv(scan.code[PP_MON_CH_P12]));
    TEST_ASSERT_INT32_WITHIN(2, -4000, pp_mon_n4_mv(scan.code[PP_MON_CH_N4]));
}

/* F-26: 27 mV per count, and the offset stored at calibration. */
static void test_monitor_vin_step_and_offset(void)
{
    TEST_ASSERT_EQUAL_INT32(0, pp_mon_vin_mv(0u, 0));
    TEST_ASSERT_EQUAL_INT32(27, pp_mon_vin_mv(1u, 0));
    TEST_ASSERT_EQUAL_INT32(2686, pp_mon_vin_mv(100u, 0));
    TEST_ASSERT_EQUAL_INT32(2586, pp_mon_vin_mv(100u, 100));
    TEST_ASSERT_EQUAL_INT32(2736, pp_mon_vin_mv(100u, -50));
}

/* F-11: the two bands of channel 2 and the limit between them. */
static void test_monitor_rail5_on_the_usb_c_input(void)
{
    static const double rails[] = {4.25, 4.75, 5.0, 5.25, 5.49};
    for (size_t i = 0u; i < (sizeof(rails) / sizeof(rails[0])); i++) {
        pp_mon_rail5_t rail = pp_mon_rail5(code_for(rails[i] * 0.4545));
        TEST_ASSERT_EQUAL(PP_SUPPLY_USB_C, rail.input);
        TEST_ASSERT_TRUE(rail.consistent);
        TEST_ASSERT_INT32_WITHIN(2, lround(rails[i] * 1000.0), rail.millivolts);
    }
}

static void test_monitor_rail5_on_the_module_input(void)
{
    static const double rails[] = {4.25, 4.75, 5.0, 5.25, 5.5};
    for (size_t i = 0u; i < (sizeof(rails) / sizeof(rails[0])); i++) {
        pp_mon_rail5_t rail = pp_mon_rail5(code_for(rails[i] * 0.2524));
        TEST_ASSERT_EQUAL(PP_SUPPLY_MODULE, rail.input);
        TEST_ASSERT_TRUE(rail.consistent);
        TEST_ASSERT_INT32_WITHIN(3, lround(rails[i] * 1000.0), rail.millivolts);
    }
}

static void test_monitor_rail5_source_changes_at_1_67_volts(void)
{
    /* Code 2736 is 1.6699 V and code 2737 is 1.6705 V. */
    pp_mon_rail5_t below = pp_mon_rail5(2736u);
    pp_mon_rail5_t above = pp_mon_rail5(2737u);
    TEST_ASSERT_EQUAL(PP_SUPPLY_MODULE, below.input);
    TEST_ASSERT_EQUAL(PP_SUPPLY_USB_C, above.input);
    TEST_ASSERT_TRUE(above.consistent);
    TEST_ASSERT_INT32_WITHIN(2, 3676, above.millivolts);
}

/* F-11: a module reading that would mean more than 5.7 V is inconsistent. */
static void test_monitor_rail5_inconsistent_above_5_7_volts(void)
{
    TEST_ASSERT_TRUE(pp_mon_rail5(code_for(5.69 * 0.2524)).consistent);
    TEST_ASSERT_FALSE(pp_mon_rail5(code_for(5.72 * 0.2524)).consistent);
    TEST_ASSERT_FALSE(pp_mon_rail5(2736u).consistent);
    /* A dead channel reads zero volts on the module scale: consistent, and low. */
    TEST_ASSERT_TRUE(pp_mon_rail5(0u).consistent);
    TEST_ASSERT_EQUAL_INT32(0, pp_mon_rail5(0u).millivolts);
}

static void test_rails_pass_with_nominal_readings(void)
{
    uint32_t reasons = pp_mon_rails_test(&scan, true, 5000);
    TEST_ASSERT_EQUAL_HEX32(0u, reasons);
    TEST_ASSERT_FALSE(pp_mon_rails_flag(reasons));
}

static void test_rails_fail_without_pwr_good(void)
{
    uint32_t reasons = pp_mon_rails_test(&scan, false, 5000);
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_PWR_GOOD, reasons);
    TEST_ASSERT_TRUE(pp_mon_rails_flag(reasons));
}

static void test_rails_fail_when_the_converter_does_not_answer(void)
{
    scan.answered = false;
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_NO_ANSWER, pp_mon_rails_test(&scan, true, 5000));
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_NO_ANSWER, pp_mon_rails_test(NULL, true, 5000));
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_NO_ANSWER | PP_RAILS_PWR_GOOD,
                            pp_mon_rails_test(NULL, false, 5000));
    TEST_ASSERT_TRUE(pp_mon_rails_flag(PP_RAILS_NO_ANSWER));
}

static void test_rails_window_of_plus_12_volts(void)
{
    static const struct {
        double volts;
        bool fails;
    } cases[] = {{11.3, true}, {11.45, false}, {12.0, false}, {12.55, false}, {12.7, true}};
    for (size_t i = 0u; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        scan.code[PP_MON_CH_P12] = code_for(cases[i].volts * 0.1754);
        uint32_t reasons = pp_mon_rails_test(&scan, true, 5000);
        TEST_ASSERT_EQUAL_HEX32(cases[i].fails ? PP_RAILS_P12 : 0u, reasons);
    }
}

static void test_rails_window_of_minus_4_volts(void)
{
    static const struct {
        double volts;
        bool fails;
    } cases[] = {{-4.4, true}, {-4.25, false}, {-4.0, false}, {-3.75, false}, {-3.6, true}};
    for (size_t i = 0u; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        scan.code[PP_MON_CH_N4] = code_for((cases[i].volts * 0.333) + 1.667);
        uint32_t reasons = pp_mon_rails_test(&scan, true, 5000);
        TEST_ASSERT_EQUAL_HEX32(cases[i].fails ? PP_RAILS_N4 : 0u, reasons);
    }
}

/* F-12: 4.25 V or more as an average, and below 5.50 V. */
static void test_rails_limits_of_the_5_volt_rail(void)
{
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_RAIL5_LOW, pp_mon_rails_test(&scan, true, 4249));
    TEST_ASSERT_EQUAL_HEX32(0u, pp_mon_rails_test(&scan, true, 4250));
    TEST_ASSERT_EQUAL_HEX32(0u, pp_mon_rails_test(&scan, true, 5499));
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_RAIL5_HIGH, pp_mon_rails_test(&scan, true, 5500));
}

/* F-12: a low 5 V rail alone is the power-limit fault and not the rails flag. */
static void test_rails_a_low_5_volt_rail_alone_does_not_set_the_flag(void)
{
    TEST_ASSERT_FALSE(pp_mon_rails_flag(PP_RAILS_RAIL5_LOW));
    TEST_ASSERT_TRUE(pp_mon_rails_flag(PP_RAILS_RAIL5_LOW | PP_RAILS_P12));
    TEST_ASSERT_TRUE(pp_mon_rails_flag(PP_RAILS_RAIL5_HIGH));
    TEST_ASSERT_TRUE(pp_mon_rails_flag(PP_RAILS_TEMP));
    TEST_ASSERT_TRUE(pp_mon_rails_flag(PP_RAILS_STUCK));
}

/* F-11: an inconsistent reading counts as a rail below its limit. */
static void test_rails_an_inconsistent_rail_reading_counts_as_low(void)
{
    scan.code[PP_MON_CH_RAIL5] = code_for(6.0 * 0.2524);
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_RAIL5_LOW, pp_mon_rails_test(&scan, true, 5000));
}

/* F-12: channel 5 between 0.3 V and 1.5 V. */
static void test_rails_window_of_the_temperature_channel(void)
{
    static const struct {
        double volts;
        bool fails;
    } cases[] = {{0.0, true}, {0.29, true}, {0.31, false}, {1.49, false}, {1.51, true}};
    for (size_t i = 0u; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        scan.code[PP_MON_CH_TEMP] = code_for(cases[i].volts);
        uint32_t reasons = pp_mon_rails_test(&scan, true, 5000);
        TEST_ASSERT_EQUAL_HEX32(cases[i].fails ? PP_RAILS_TEMP : 0u, reasons);
    }
}

/* F-12: no rail channel at code 0 or 4095. */
static void test_rails_a_rail_channel_at_the_end_of_its_scale_fails(void)
{
    static const uint8_t rail_channels[] = {PP_MON_CH_RAIL5, PP_MON_CH_P12, PP_MON_CH_N4};
    static const uint16_t ends[] = {0u, 4095u};
    for (size_t channel = 0u; channel < sizeof(rail_channels); channel++) {
        for (size_t end = 0u; end < 2u; end++) {
            nominal_scan();
            scan.code[rail_channels[channel]] = ends[end];
            uint32_t reasons = pp_mon_rails_test(&scan, true, 5000);
            TEST_ASSERT_BITS_HIGH(PP_RAILS_STUCK, reasons);
            TEST_ASSERT_TRUE(pp_mon_rails_flag(reasons));
        }
    }
    /* The other channels may rest at zero: a CC pin without a cable does. */
    nominal_scan();
    scan.code[PP_MON_CH_CC1] = 0u;
    scan.code[PP_MON_CH_VOUT] = 0u;
    scan.code[PP_MON_CH_VIN] = 0u;
    TEST_ASSERT_EQUAL_HEX32(0u, pp_mon_rails_test(&scan, true, 5000));
}

static void test_rails_reasons_combine(void)
{
    scan.code[PP_MON_CH_P12] = code_for(9.0 * 0.1754);
    scan.code[PP_MON_CH_TEMP] = code_for(2.0);
    TEST_ASSERT_EQUAL_HEX32(PP_RAILS_PWR_GOOD | PP_RAILS_P12 | PP_RAILS_TEMP | PP_RAILS_RAIL5_LOW,
                            pp_mon_rails_test(&scan, false, 4000));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_monitor_pin_voltage_of_a_code);
    RUN_TEST(test_monitor_scales_agree_with_the_specification_for_every_code);
    RUN_TEST(test_monitor_nominal_values);
    RUN_TEST(test_monitor_vin_step_and_offset);
    RUN_TEST(test_monitor_rail5_on_the_usb_c_input);
    RUN_TEST(test_monitor_rail5_on_the_module_input);
    RUN_TEST(test_monitor_rail5_source_changes_at_1_67_volts);
    RUN_TEST(test_monitor_rail5_inconsistent_above_5_7_volts);
    RUN_TEST(test_rails_pass_with_nominal_readings);
    RUN_TEST(test_rails_fail_without_pwr_good);
    RUN_TEST(test_rails_fail_when_the_converter_does_not_answer);
    RUN_TEST(test_rails_window_of_plus_12_volts);
    RUN_TEST(test_rails_window_of_minus_4_volts);
    RUN_TEST(test_rails_limits_of_the_5_volt_rail);
    RUN_TEST(test_rails_a_low_5_volt_rail_alone_does_not_set_the_flag);
    RUN_TEST(test_rails_an_inconsistent_rail_reading_counts_as_low);
    RUN_TEST(test_rails_window_of_the_temperature_channel);
    RUN_TEST(test_rails_a_rail_channel_at_the_end_of_its_scale_fails);
    RUN_TEST(test_rails_reasons_combine);
    return UNITY_END();
}
