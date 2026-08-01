#include "fft.h"
#include <math.h>

static complex32_t complex_multiply(complex32_t left, complex32_t right)
{
    complex32_t result;
    result.real = left.real * right.real - left.imaginary * right.imaginary;
    result.imaginary = left.real * right.imaginary + left.imaginary * right.real;
    return result;
}

static void fft_transform(complex32_t *data, size_t data_count, float32_t direction)
{
    ASSERT(data != NULL);
    ASSERT(data_count > 0);
    ASSERT((data_count & (data_count - 1)) == 0);

    if (data_count < 2)
    {
        return;
    }

    size_t target_index = 0;
    for (size_t index = 1; index < data_count; ++index)
    {
        size_t bit_mask = data_count >> 1;
        while (target_index & bit_mask)
        {
            target_index ^= bit_mask;
            bit_mask >>= 1;
        }
        target_index ^= bit_mask;

        if (index < target_index)
        {
            complex32_t temporary = data[index];
            data[index] = data[target_index];
            data[target_index] = temporary;
        }
    }

    for (size_t stage_length = 2; stage_length <= data_count; stage_length <<= 1)
    {
        size_t half_length = stage_length / 2;
        float32_t angle = direction * 2.0f * (float32_t)M_PI / (float32_t)stage_length;
        complex32_t twiddle_step = { cosf(angle), sinf(angle) };

        for (size_t group_start = 0; group_start < data_count; group_start += stage_length)
        {
            complex32_t twiddle = { 1.0f, 0.0f };

            for (size_t pair_index = 0; pair_index < half_length; ++pair_index)
            {
                complex32_t even_value = data[group_start + pair_index];
                complex32_t odd_value  = data[group_start + pair_index + half_length];
                complex32_t odd_rotated = complex_multiply(odd_value, twiddle);

                data[group_start + pair_index].real =
                    even_value.real + odd_rotated.real;
                data[group_start + pair_index].imaginary =
                    even_value.imaginary + odd_rotated.imaginary;

                data[group_start + pair_index + half_length].real =
                    even_value.real - odd_rotated.real;
                data[group_start + pair_index + half_length].imaginary =
                    even_value.imaginary - odd_rotated.imaginary;

                twiddle = complex_multiply(twiddle, twiddle_step);
            }
        }
    }
}

void fft_radix2(complex32_t *data, size_t data_count)
{
    fft_transform(data, data_count, -1.0f);
}

void ifft_radix2(complex32_t *data, size_t data_count)
{
    fft_transform(data, data_count, 1.0f);

    float32_t inverse_scale = 1.0f / (float32_t)data_count;
    for (size_t index = 0; index < data_count; ++index)
    {
        data[index].real *= inverse_scale;
        data[index].imaginary *= inverse_scale;
    }
}

void fft_shift(complex32_t *data, size_t data_count)
{
    ASSERT(data != NULL);
    ASSERT((data_count & 1) == 0);

    size_t half_length = data_count / 2;
    for (size_t index = 0; index < half_length; ++index)
    {
        complex32_t temporary = data[index];
        data[index] = data[index + half_length];
        data[index + half_length] = temporary;
    }
}