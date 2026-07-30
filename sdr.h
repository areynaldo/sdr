#ifndef SDR_H
#define SDR_H

#include "base.h"

size_t uint8_buffer_to_float32_buffer(
    uint8_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_count);

void demodulate_fm_float32(
    float32_t *iq_buffer,
    size_t iq_buffer_count,
    float32_t *output_buffer,
    size_t output_buffer_count);

size_t decimate_block_average(
    float32_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_capacity,
    uint32_t factor);

size_t normalize_rads_float32(
    float32_t *input_buffer,
    size_t input_count);

size_t float32_rads_to_int16_audio(
    float32_t *input_buffer,
    size_t input_count,
    int16_t *output_buffer,
    size_t output_count,
    float32_t gain);

#endif
