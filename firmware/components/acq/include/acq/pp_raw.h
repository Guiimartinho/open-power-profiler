/*
 * The raw word of the capture program (sections 4.6 and 4.7 of the
 * specification).
 *
 * For every sample the capture program shifts two pins into one 32-bit word,
 * sixteen times, most significant pair first: the data pin of the converter
 * and the data pin of the shift registers. The converter is the lower pin of
 * the pair, so its bits land on the even positions and the side bits on the
 * odd ones. Bit 30 is the most significant bit of the conversion result and
 * bit 31 the first side bit.
 *
 * The sixteen side bits, in the order of section 4.7, with the first one
 * shifted as bit 15:
 *
 *     15..14  range (address of the multiplexer)
 *     13      output switch requested on
 *     12      VIN above the over-voltage level
 *     11      over-current comparator
 *     10      jump-up comparator
 *      9      step-up comparator
 *      8      carrier rails present
 *      7..0   logic inputs D7 to D0
 *
 * These functions run once per sample in the acquisition path, so they are
 * inline and free of branches.
 */
#ifndef PP_RAW_H
#define PP_RAW_H

#include <stdbool.h>
#include <stdint.h>

#include "proto/pp_proto_defs.h"

#define PP_SIDE_RANGE_SHIFT 14u
#define PP_SIDE_RANGE_MASK 0x3u
#define PP_SIDE_OUTPUT_BIT ((uint16_t)(1u << 13))
#define PP_SIDE_VIN_OV_BIT ((uint16_t)(1u << 12))
#define PP_SIDE_CMP_OC_BIT ((uint16_t)(1u << 11))
#define PP_SIDE_CMP_JUMP_BIT ((uint16_t)(1u << 10))
#define PP_SIDE_CMP_UP_BIT ((uint16_t)(1u << 9))
#define PP_SIDE_PWR_GOOD_BIT ((uint16_t)(1u << 8))
#define PP_SIDE_LOGIC_MASK 0xFFu

/* Gather the sixteen bits on the even positions of a word. */
static inline uint16_t pp_raw_even_bits(uint32_t value)
{
    value &= 0x55555555u;
    value = (value | (value >> 1)) & 0x33333333u;
    value = (value | (value >> 2)) & 0x0F0F0F0Fu;
    value = (value | (value >> 4)) & 0x00FF00FFu;
    value = (value | (value >> 8)) & 0x0000FFFFu;
    return (uint16_t)value;
}

/* Spread sixteen bits onto the even positions of a word. */
static inline uint32_t pp_raw_spread_bits(uint16_t value)
{
    uint32_t wide = value;
    wide = (wide | (wide << 8)) & 0x00FF00FFu;
    wide = (wide | (wide << 4)) & 0x0F0F0F0Fu;
    wide = (wide | (wide << 2)) & 0x33333333u;
    wide = (wide | (wide << 1)) & 0x55555555u;
    return wide;
}

/* The conversion result of a raw word. */
static inline uint16_t pp_raw_adc(uint32_t raw)
{
    return pp_raw_even_bits(raw);
}

/* The sixteen side bits of a raw word. */
static inline uint16_t pp_raw_side(uint32_t raw)
{
    return pp_raw_even_bits(raw >> 1);
}

/*
 * The raw word that the capture program would deliver for a result and its
 * side bits. The fakes of the tests and the device twin build their input
 * with it.
 */
static inline uint32_t pp_raw_pack(uint16_t adc, uint16_t side)
{
    return pp_raw_spread_bits(adc) | (pp_raw_spread_bits(side) << 1);
}

static inline uint8_t pp_side_range(uint16_t side)
{
    return (uint8_t)((side >> PP_SIDE_RANGE_SHIFT) & PP_SIDE_RANGE_MASK);
}

static inline uint8_t pp_side_logic(uint16_t side)
{
    return (uint8_t)(side & PP_SIDE_LOGIC_MASK);
}

static inline bool pp_side_output_on(uint16_t side)
{
    return (side & PP_SIDE_OUTPUT_BIT) != 0u;
}

static inline bool pp_side_cmp_jump(uint16_t side)
{
    return (side & PP_SIDE_CMP_JUMP_BIT) != 0u;
}

/*
 * True for an over-range sample: its code is at full scale, or the jump
 * comparator was set when the sample was taken (rule F-35). In a locked
 * range the code alone is not enough: the shunt voltage can lie between the
 * full scale of the converter and the jump level.
 */
static inline bool pp_raw_is_over_range(uint16_t adc, uint16_t side)
{
    return (adc == UINT16_MAX) || pp_side_cmp_jump(side);
}

/*
 * The sample word of section 7.3 for a result and its side bits: code, range,
 * logic inputs, the request of the output switch and the over-range mark.
 * The other marks (invalid, fault, under-range) are left clear; the
 * acquisition rules set them.
 */
static inline uint32_t pp_raw_sample_word(uint16_t adc, uint16_t side)
{
    return ((uint32_t)adc << PP_SAMPLE_ADC_SHIFT) |
           ((uint32_t)pp_side_range(side) << PP_SAMPLE_RANGE_SHIFT) |
           ((uint32_t)pp_raw_is_over_range(adc, side) << PP_SAMPLE_OVER_RANGE_SHIFT) |
           ((uint32_t)pp_side_output_on(side) << PP_SAMPLE_OUTPUT_SHIFT) |
           ((uint32_t)pp_side_logic(side) << PP_SAMPLE_LOGIC_SHIFT);
}

#endif /* PP_RAW_H */
