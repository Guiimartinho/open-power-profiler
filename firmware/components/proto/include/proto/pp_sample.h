/*
 * The 32-bit sample word (section 7.3 of the specification).
 *
 * Every sample carries its own context: the raw ADC code, the range that was
 * active, whether the sample falls in a settling window, the fault latch, the
 * over-range and under-range marks, the request of the output switch and the
 * eight logic inputs. Field positions come from pp_proto_defs.h.
 *
 * These functions run once per sample in the acquisition path, so they are
 * inline and free of branches.
 */
#ifndef PP_SAMPLE_H
#define PP_SAMPLE_H

#include <stdbool.h>
#include <stdint.h>

#include "proto/pp_proto_defs.h"

/* The flag bits in their position inside the word. */
#define PP_SAMPLE_INVALID_BIT ((uint32_t)PP_SAMPLE_INVALID_MASK << PP_SAMPLE_INVALID_SHIFT)
#define PP_SAMPLE_FAULT_BIT ((uint32_t)PP_SAMPLE_FAULT_MASK << PP_SAMPLE_FAULT_SHIFT)
#define PP_SAMPLE_OVER_RANGE_BIT ((uint32_t)PP_SAMPLE_OVER_RANGE_MASK << PP_SAMPLE_OVER_RANGE_SHIFT)
#define PP_SAMPLE_UNDER_RANGE_BIT                                                                  \
    ((uint32_t)PP_SAMPLE_UNDER_RANGE_MASK << PP_SAMPLE_UNDER_RANGE_SHIFT)
#define PP_SAMPLE_OUTPUT_BIT ((uint32_t)PP_SAMPLE_OUTPUT_MASK << PP_SAMPLE_OUTPUT_SHIFT)

typedef struct {
    uint16_t adc;     /* raw ADC code */
    uint8_t range;    /* active range, 0 to PP_RANGE_COUNT - 1 */
    bool invalid;     /* inside a settling window: range change or change of supply */
    bool fault;       /* over-current trip latched */
    bool over_range;  /* code at full scale, or the jump comparator set (F-35) */
    bool under_range; /* code below the pedestal (F-22) */
    bool output;      /* the output switch is requested on */
    uint8_t logic;    /* digital inputs D0 to D7 */
} pp_sample_t;

/* Build the word. A value wider than its field is truncated to the field. */
static inline uint32_t pp_sample_pack(const pp_sample_t *sample)
{
    return (((uint32_t)sample->adc & PP_SAMPLE_ADC_MASK) << PP_SAMPLE_ADC_SHIFT) |
           (((uint32_t)sample->range & PP_SAMPLE_RANGE_MASK) << PP_SAMPLE_RANGE_SHIFT) |
           ((uint32_t)sample->invalid << PP_SAMPLE_INVALID_SHIFT) |
           ((uint32_t)sample->fault << PP_SAMPLE_FAULT_SHIFT) |
           ((uint32_t)sample->over_range << PP_SAMPLE_OVER_RANGE_SHIFT) |
           ((uint32_t)sample->under_range << PP_SAMPLE_UNDER_RANGE_SHIFT) |
           ((uint32_t)sample->output << PP_SAMPLE_OUTPUT_SHIFT) |
           (((uint32_t)sample->logic & PP_SAMPLE_LOGIC_MASK) << PP_SAMPLE_LOGIC_SHIFT);
}

static inline uint16_t pp_sample_adc(uint32_t word)
{
    return (uint16_t)((word >> PP_SAMPLE_ADC_SHIFT) & PP_SAMPLE_ADC_MASK);
}

static inline uint8_t pp_sample_range(uint32_t word)
{
    return (uint8_t)((word >> PP_SAMPLE_RANGE_SHIFT) & PP_SAMPLE_RANGE_MASK);
}

static inline bool pp_sample_is_invalid(uint32_t word)
{
    return (word & PP_SAMPLE_INVALID_BIT) != 0u;
}

static inline bool pp_sample_has_fault(uint32_t word)
{
    return (word & PP_SAMPLE_FAULT_BIT) != 0u;
}

static inline bool pp_sample_is_over_range(uint32_t word)
{
    return (word & PP_SAMPLE_OVER_RANGE_BIT) != 0u;
}

static inline bool pp_sample_is_under_range(uint32_t word)
{
    return (word & PP_SAMPLE_UNDER_RANGE_BIT) != 0u;
}

static inline bool pp_sample_output_on(uint32_t word)
{
    return (word & PP_SAMPLE_OUTPUT_BIT) != 0u;
}

/*
 * True for a sample that the host does not use as a measurement: inside a
 * settling window, over-range or under-range (section 9).
 */
static inline bool pp_sample_is_flagged(uint32_t word)
{
    return (word &
            (PP_SAMPLE_INVALID_BIT | PP_SAMPLE_OVER_RANGE_BIT | PP_SAMPLE_UNDER_RANGE_BIT)) != 0u;
}

static inline uint8_t pp_sample_logic(uint32_t word)
{
    return (uint8_t)((word >> PP_SAMPLE_LOGIC_SHIFT) & PP_SAMPLE_LOGIC_MASK);
}

/* Split the word into its fields. The reserved bits are ignored. */
static inline void pp_sample_unpack(uint32_t word, pp_sample_t *sample)
{
    sample->adc = pp_sample_adc(word);
    sample->range = pp_sample_range(word);
    sample->invalid = pp_sample_is_invalid(word);
    sample->fault = pp_sample_has_fault(word);
    sample->over_range = pp_sample_is_over_range(word);
    sample->under_range = pp_sample_is_under_range(word);
    sample->output = pp_sample_output_on(word);
    sample->logic = pp_sample_logic(word);
}

#endif /* PP_SAMPLE_H */
