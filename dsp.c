#include "dsp.h"

size_t uint8_buffer_to_float32_buffer(
    uint8_t   *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity
)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);

    size_t output_count = MIN(input_count, output_capacity);
    for (size_t i = 0; i < output_count; ++i)
    {
        int8_t centered  = (int8_t)(input_buffer[i] ^ 0x80);
        output_buffer[i] = (float32_t)centered;
    }
    return output_count;
}

size_t demodulate_fm_float32(
    float32_t *iq_buffer,
    size_t     iq_buffer_count,
    float32_t *output_buffer,
    size_t     output_capacity
)
{
    ASSERT(iq_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT((iq_buffer_count & 1) == 0);

    if (iq_buffer_count < 4)
    {
        return 0;
    }

    // one output per consecutive complex pair: (count/2) samples -> (count/2)-1 outputs
    size_t available    = (iq_buffer_count / 2) - 1;
    size_t output_count = MIN(available, output_capacity);

    for (size_t i = 0; i < output_count; ++i)
    {
        size_t    idx    = i * 2;
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

size_t demodulate_am_float32(
    float32_t *iq_buffer,
    size_t     iq_buffer_count,
    float32_t *output_buffer,
    size_t     output_capacity
)
{
    ASSERT(iq_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT((iq_buffer_count & 1) == 0);

    size_t n = (iq_buffer_count / 2 < output_capacity) ? iq_buffer_count / 2 : output_capacity;
    for (size_t i = 0; i < n; i++)
    {
        float32_t I = iq_buffer[2 * i];
        float32_t Q = iq_buffer[2 * i + 1];
        output_buffer[i] = sqrtf(I * I + Q * Q);
    }
    return n;
}

// TODO(areynaldo): benchmark and SIMD test
// TODO(areynaldo): benchmark unrolling
size_t decimate_block_average_float32(
    float32_t *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity,
    uint32_t   factor
)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(factor >= 1);

    size_t available    = input_count / factor;
    size_t output_count = MIN(available, output_capacity);

    float32_t inv_factor = 1.0f / (float32_t)factor;
    for (size_t out = 0; out < output_count; ++out)
    {
        size_t    base        = out * factor;
        float32_t accumulator = 0.0f;
        for (uint32_t j = 0; j < factor; ++j)
        {
            accumulator += input_buffer[base + j];
        }
        output_buffer[out] = accumulator * inv_factor;
    }
    return output_count;
}

size_t float32_rads_to_float32_audio(
    float32_t *input_buffer,
    size_t     input_count,
    float32_t *output_buffer,
    size_t     output_capacity,
    float32_t  gain
)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);
    ASSERT(gain >= 0);

    size_t output_count = MIN(input_count, output_capacity);
    for (size_t i = 0; i < output_count; ++i)
    {
        output_buffer[i] = input_buffer[i] * gain;
    }
    return output_count;
}

size_t float32_audio_to_int16_audio(
    float32_t *input_buffer,
    size_t     input_count,
    int16_t   *output_buffer,
    size_t     output_capacity
)
{
    ASSERT(input_buffer != NULL);
    ASSERT(output_buffer != NULL);

    size_t output_count = MIN(input_count, output_capacity);
    for (size_t i = 0; i < output_count; ++i)
    {
        float32_t f      = input_buffer[i];
        f                = f < -1.0f ? -1.0f : (f > 1.0f ? 1.0f : f);
        output_buffer[i] = (int16_t)(f * 32767.0f);
    }

    return output_count;
}

// Window
float32_t window_coefficient(window_type_t type, size_t n, size_t taps_count)
{
    ASSERT(taps_count >= 1);
    if (taps_count == 1)
        return 1.0f;

    float32_t N     = (float32_t)(taps_count - 1);
    float32_t ratio = (float32_t)n / N;
    float32_t theta = 2.0f * (float32_t)M_PI * ratio;

    switch (type)
    {
    case WINDOW_HAMMING:
        return 0.54f - 0.46f * cosf(theta);
    case WINDOW_HANN:
        return 0.5f - 0.5f * cosf(theta);
    case WINDOW_BLACKMAN_HARRIS:
        return 0.35875f - 0.48829f * cosf(theta) + 0.14128f * cosf(2.0f * theta) - 0.01168f * cosf(3.0f * theta);
    case WINDOW_RECTANGULAR:
    default:
        return 1.0f;
    }
}

void window_apply(float32_t *buffer, size_t count, window_type_t type)
{
    for (size_t n = 0; n < count; n++)
    {
        buffer[n] *= window_coefficient(type, n, count);
    }
}

float32_t window_coherent_gain(window_type_t type, size_t count)
{
    ASSERT(count >= 1);
    float32_t sum = 0.0f;
    for (size_t n = 0; n < count; n++)
    {
        sum += window_coefficient(type, n, count);
    }
    return sum / (float32_t)count;
}

size_t biquad_filter_process(biquad_filter_t *filter, float32_t *input, size_t input_count)
{
    ASSERT(filter != NULL);
    ASSERT(input != NULL);
    for (size_t i = 0; i < input_count; i++)
    {
        float32_t x = input[i];
        float32_t y = filter->b0 * x + filter->z1;
        filter->z1  = filter->b1 * x - filter->a1 * y + filter->z2;
        filter->z2  = filter->b2 * x - filter->a2 * y;
    }
    return input_count;
}

biquad_filter_t biquad_filter_design_deemphasis(float32_t sample_rate, float32_t tau_seconds)
{
    float32_t delta = 1.0f / sample_rate;
    float32_t alpha = delta / (tau_seconds + delta);

    biquad_filter_t filter = {0};

    filter.b0 = alpha;
    filter.b1 = 0.0f;
    filter.b2 = 0.0f;

    filter.a1 = -(1.0f - alpha);
    filter.a2 = 0.0f;

    return filter;
}

biquad_filter_t biquad_filter_design_dc_block(float32_t sample_rate, float32_t hz)
{
    float32_t w0 = 2.0f * (float32_t)M_PI * hz / sample_rate;
    float32_t r  = 1.0f - w0;

    biquad_filter_t filter = {0};

    filter.b0 = 1.0f;
    filter.b1 = -1.0f;
    filter.b2 = 0.0f;

    filter.a1 = -r;
    filter.a2 = 0.0f;

    return filter;
}

biquad_filter_t biquad_filter_design_notch(float32_t sample_rate, float32_t frequency, float32_t q)
{
    float32_t w0    = 2.0f * (float32_t)M_PI * frequency / sample_rate;
    float32_t cw    = cosf(w0);
    float32_t alpha = sinf(w0) / (2.0f * q);

    float32_t a0 = 1.0f + alpha;

    biquad_filter_t filter = {0};

    filter.b0 = 1.0f / a0;
    filter.b1 = -2.0f * cw / a0;
    filter.b2 = 1.0f / a0;

    filter.a1 = -2.0f * cw / a0;
    filter.a2 = (1.0f - alpha) / a0;

    return filter;
}

// Filter
fir_filter_t fir_filter_init(float32_t *taps, size_t taps_count, uint32_t decimation)
{
    ASSERT(taps != NULL);
    ASSERT(taps_count >= 1);
    ASSERT(decimation >= 1);

    fir_filter_t filter     = {0};
    filter.taps_count       = taps_count;
    filter.decimation       = decimation;
    filter.phase            = 0;
    filter.history_position = 0;
    filter.taps             = malloc(taps_count * sizeof(float32_t));
    filter.history          = calloc(taps_count, sizeof(float32_t));
    memcpy(filter.taps, taps, taps_count * sizeof(float32_t));
    return filter;
}

void fir_filter_free(fir_filter_t *filter)
{
    if (filter == NULL)
    {
        return;
    }
    free(filter->taps);
    free(filter->history);
    filter->taps    = NULL;
    filter->history = NULL;
}

size_t fir_filter_process(
    fir_filter_t *filter,
    float32_t    *input,
    size_t        input_count,
    float32_t    *output,
    size_t        output_capacity
)
{
    ASSERT(filter != NULL);
    ASSERT(input != NULL);
    ASSERT(output != NULL);

    size_t output_count = 0;
    for (size_t i = 0; i < input_count; i++)
    {
        filter->history[filter->history_position] = input[i];
        filter->history_position                  = (filter->history_position + 1) % filter->taps_count;

        filter->phase++;
        if (filter->phase < filter->decimation)
        {
            continue;
        }
        filter->phase = 0;
        if (output_count >= output_capacity)
        {
            break;
        }

        float32_t accumulator = 0.0f;
        size_t    index       = filter->history_position;
        for (size_t t = 0; t < filter->taps_count; t++)
        {
            if (index == 0)
            {
                index = filter->taps_count - 1;
            }
            else
            {
                index = index - 1;
            }
            accumulator += filter->taps[t] * filter->history[index];
        }
        output[output_count] = accumulator;
        output_count++;
    }
    return output_count;
}

void fir_design_lowpass(float32_t *taps, size_t taps_count, float32_t cutoff_normalized, window_type_t window)
{
    ASSERT(taps != NULL);
    ASSERT(taps_count >= 1);

    float32_t sum    = 0.0f;
    float32_t center = (float32_t)(taps_count - 1) / 2.0f;

    for (size_t i = 0; i < taps_count; i++)
    {
        float32_t k = (float32_t)i - center;

        float32_t sinc;
        if (k == 0.0f)
        {
            sinc = 2.0f * cutoff_normalized;
        }
        else
        {
            float32_t x = 2.0f * (float32_t)M_PI * cutoff_normalized * k;
            sinc        = sinf(x) / ((float32_t)M_PI * k);
        }

        taps[i] = sinc * window_coefficient(window, i, taps_count);
        sum += taps[i];
    }

    // normalize so the filter doesn't change signal level
    for (size_t i = 0; i < taps_count; i++)
    {
        taps[i] /= sum;
    }
}

static void fir_design_highpass(float32_t *taps, size_t taps_count, float32_t cutoff_normalized, window_type_t window)
{
    fir_design_lowpass(taps, taps_count, cutoff_normalized, window);
    for (size_t n = 0; n < taps_count; n++)
    {
        taps[n] = -taps[n];
    }
    if (taps_count % 2 == 1)
    {
        taps[(taps_count - 1) / 2] += 1.0f;
    }
}