#ifndef SDR_H
#define SDR_H

#include "base.h"

size_t uint8_buffer_to_float32_buffer(
    uint8_t   *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity
);

size_t demodulate_fm_float32(
    float32_t *iq_buffer,
    size_t     iq_buffer_count,
    float32_t *output_buffer,
    size_t     output_capacity
);

size_t demodulate_am_float32(
    float32_t *iq_buffer,
    size_t     iq_buffer_count,
    float32_t *output_buffer,
    size_t     output_capacity
);

size_t decimate_block_average_float32(
    float32_t *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity,
    uint32_t   factor
);

size_t float32_rads_to_float32_audio(
    float32_t *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity,
    float32_t  gain
);

size_t float32_audio_to_int16_audio(
    float32_t *input_buffer,
    size_t     input_count,
    int16_t   *output_buffer,
    size_t     output_capacity
);

// Windows
typedef enum window_type_t
{
    WINDOW_RECTANGULAR,
    WINDOW_HAMMING,
    WINDOW_HANN,
    WINDOW_BLACKMAN_HARRIS,
} window_type_t;

float32_t window_coefficient(window_type_t type, size_t n, size_t taps_count);

float32_t window_coherent_gain(window_type_t type, size_t count);

// Filters
typedef struct biquad_filter_t
{
    float32_t b0, b1, b2; // feedforward
    float32_t a1, a2;     // feedback
    float32_t z1, z2;     // state
} biquad_filter_t;

size_t biquad_filter_process(biquad_filter_t *filter, float32_t *input, size_t input_count);

biquad_filter_t biquad_filter_design_deemphasis(float32_t sample_rate, float32_t tau_seconds);

biquad_filter_t biquad_filter_design_dc_block(float32_t sample_rate, float32_t hz);

biquad_filter_t biquad_filter_design_notch(float32_t sample_rate, float32_t frequency, float32_t q);

typedef struct fir_filter_t
{
    float32_t *taps;
    size_t     taps_count;
    float32_t *history;
    size_t     history_position;
    uint32_t   decimation;
    uint32_t   phase;
} fir_filter_t;

fir_filter_t fir_filter_init(float32_t *taps, size_t num_taps, uint32_t decimation);

void fir_filter_free(fir_filter_t *filter);

void fir_design_lowpass(float32_t *taps, size_t taps_count, float32_t cutoff_normalized, window_type_t window);

size_t fir_filter_process(
    fir_filter_t *filter,
    float32_t    *input,
    size_t        input_count,
    float32_t    *output,
    size_t        output_capacity
);

#endif
