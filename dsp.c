#include "dsp.h"

static inline size_t size_min(size_t a, size_t b)
{
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

size_t uint8_buffer_to_float32_buffer(
    uint8_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_capacity)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);

    size_t output_count = size_min(input_count, output_capacity);
    for (size_t i = 0; i < output_count; ++i)
    {
        int8_t centered = (int8_t)(input_buffer[i] ^ 0x80);
        output_buffer[i] = (float32_t)centered;
    }
    return output_count;
}

size_t demodulate_fm_float32(
    float32_t *iq_buffer,
    size_t iq_buffer_count,
    float32_t *output_buffer,
    size_t output_capacity)
{
    ASSERT(iq_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT((iq_buffer_count & 1) == 0);

    if (iq_buffer_count < 4)
    {
        return 0;
    }

    // one output per consecutive complex pair: (count/2) samples -> (count/2)-1 outputs
    size_t available = (iq_buffer_count / 2) - 1;
    size_t output_count = size_min(available, output_capacity);

    for (size_t i = 0; i < output_count; ++i)
    {
        size_t idx = i * 2;
        float32_t i_prev = iq_buffer[idx];
        float32_t q_prev = iq_buffer[idx + 1];
        float32_t i_next = iq_buffer[idx + 2];
        float32_t q_next = iq_buffer[idx + 3];

        float32_t real      = i_prev * i_next + q_prev * q_next;
        float32_t imaginary = i_prev * q_next - q_prev * i_next;

        output_buffer[i] = atan2f(imaginary, real);
    }
    return output_count;
}

// TODO(areynaldo): benchmark and SIMD test
// TODO(areynaldo): benchmark unrolling
size_t decimate_block_average_float32(
    float32_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_capacity,
    uint32_t factor)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(factor >= 1);

    size_t available = input_count / factor;
    size_t output_count = size_min(available, output_capacity);

    float32_t inv_factor = 1.0f / (float32_t)factor;
    for (size_t out = 0; out < output_count; ++out)
    {
        size_t base = out * factor;
        float32_t accumulator = 0.0f;
        for (uint32_t j = 0; j < factor; ++j)
        {
            accumulator += input_buffer[base + j];
        }
        output_buffer[out] = accumulator * inv_factor;
    }
    return output_count;
}

size_t float32_rads_to_int16_audio(
    float32_t *input_buffer,
    size_t input_count,
    int16_t *output_buffer,
    size_t output_capacity,
    float32_t gain)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(gain >= 0);

    size_t output_count = size_min(input_count, output_capacity);
    for (size_t i = 0; i < output_count; ++i)
    {
        float32_t sample = input_buffer[i] * gain;
        sample = CLAMP(sample, -32768.0f, 32767.0f);
        output_buffer[i] = (int16_t)sample;
    }
    return output_count;
}