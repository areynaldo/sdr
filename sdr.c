#include "sdr.h"

size_t uint8_buffer_to_float32_buffer(
    uint8_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_count)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(output_count >= input_count);

    size_t i = 0;
    while (i < input_count && i < output_count)
    {
        int8_t centered = (int8_t)(input_buffer[i] ^ 0x80);
        output_buffer[i] = (float32_t)centered;
        i++;
    }

    return i;
}

void demodulate_fm_float32(
    float32_t *iq_buffer,
    size_t iq_buffer_count,
    float32_t *output_buffer,
    size_t output_buffer_count)
{
    ASSERT(iq_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT((iq_buffer_count & 1) == 0);
    ASSERT(iq_buffer_count >= 4);
    ASSERT(output_buffer_count <= (iq_buffer_count / 2) - 1);

    for (size_t i = 0; i < output_buffer_count; ++i)
    {
        size_t idx = i * 2;
        float32_t i_prev = iq_buffer[idx];
        float32_t q_prev = iq_buffer[idx + 1];
        float32_t i_next = iq_buffer[idx + 2];
        float32_t q_next = iq_buffer[idx + 3];

        float32_t real = i_prev * i_next + q_prev * q_next;
        float32_t imaginary = i_prev * q_next - q_prev * i_next;
        output_buffer[i] = atan2f(imaginary, real);
    }
}

// TODO(areynaldo): benchmark and SIMD test
// TODO(areynaldo): benchmark unrolling
size_t decimate_block_average_float32(
    const float32_t *input_buffer,
    size_t input_count,
    float32_t *output_buffer,
    size_t output_capacity,
    uint32_t factor)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(factor >= 1);

    size_t output_count = input_count / factor;
    ASSERT(output_count <= output_capacity);

    const float32_t inv_factor = 1.0f / (float32_t)factor;

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

size_t normalize_rads_float32(float32_t *input_buffer, size_t input_buffer_count)
{
    ASSERT(input_buffer != NULL);

    const float32_t inv_pi = 1.0f / M_PI;
    for (size_t i = 0; i < input_buffer_count; ++i)
    {
        input_buffer[i] *= inv_pi;
    }
    return input_buffer_count;
}

size_t float32_rads_to_int16_audio(
    float32_t *input_buffer,
    size_t input_count,
    int16_t *output_buffer,
    size_t output_count,
    float32_t gain)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(output_count <= input_count);
    ASSERT(gain >= 0);

    for (size_t i = 0; i < output_count; ++i)
    {
        float32_t sample = (input_buffer[i] * gain);
        sample = CLAMP(sample, -32768.0f, 32767.0f);
        output_buffer[i] = (int16_t)sample;
    }

    return output_count;
}