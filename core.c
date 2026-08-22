#include "core.h"
#include "fft.h"

core_t sdr_core_init(core_t settings)
{
    if (settings.audio_gain == 0)
    {
        settings.audio_gain = SDR_CORE_AUDIO_GAIN_DEFAULT;
    }

    if (settings.center_freq == 0)
    {
        settings.center_freq = SDR_CORE_CENTER_FREQ_DEFAULT;
    }

    if (settings.sample_rate == 0)
    {
        settings.sample_rate = SDR_CORE_SAMPLE_RATE_DEFAULT;
    }

    if (settings.iq_pairs_count == 0)
    {
        settings.iq_pairs_count = SDR_CORE_IQ_PAIRS_DEFAULT;
    }

    if (settings.iq_pairs.capacity == 0)
    {
        settings.iq_pairs.capacity = 2 * settings.iq_pairs_count;
    }

    if (settings.device_iq_pairs.capacity == 0)
    {
        settings.device_iq_pairs.capacity = 2 * settings.iq_pairs_count;
    }

    if (settings.iq_pairs.data == NULL)
    {
        settings.device_iq_pairs.data = (uint8_t *)malloc(settings.iq_pairs.capacity * sizeof(uint8_t));
        if (settings.device_iq_pairs.data == NULL)
        {
            printf("Failed to allocate device iq pairs buffer.\n");
            return settings;
        }
        settings.iq_pairs.data = (float32_t *)malloc(settings.iq_pairs.capacity * sizeof(float32_t));
        if (settings.iq_pairs.data == NULL)
        {
            printf("Failed to allocate iq pairs buffer.\n");
            return settings;
        }
    }

    if (settings.device == 0)
    {
        if (rtlsdr_open(&settings.device, 0) < 0)
        {
            printf("Failed to open device 0.\n");
            settings.device = 0;
            return settings;
        }
    }

    rtlsdr_set_tuner_gain_mode(settings.device, 0);
    rtlsdr_set_sample_rate(settings.device, settings.sample_rate);
    rtlsdr_set_center_freq(settings.device, settings.center_freq);
    rtlsdr_reset_buffer(settings.device);

    return settings;
}

core_error_t sdr_core_read_iq_pairs_sync(core_t *core)
{
    ASSERT(core != NULL);
    ASSERT(core->device != NULL);
    ASSERT(core->device_iq_pairs.data != NULL);
    ASSERT(core->iq_pairs.data != NULL);

    int read_count       = 0;
    core->iq_pairs.count = 0;
    if (rtlsdr_read_sync(core->device, core->device_iq_pairs.data, core->iq_pairs.capacity, &read_count) < 0 ||
        read_count == 0)
    {
        return CORE_ERROR_READ_SYNC_FAILED;
    }

    core->iq_pairs.count = (size_t)read_count;
    uint8_buffer_to_float32_buffer(
        core->device_iq_pairs.data,
        core->iq_pairs.count,
        core->iq_pairs.data,
        core->iq_pairs.count
    );
    return CORE_ERROR_NONE;
}

void sdr_core_set_center_freq(core_t *core, float32_t freq)
{
    ASSERT(core != NULL);
    ASSERT(core->device != NULL);
    ASSERT(core->device_iq_pairs.data != NULL);
    ASSERT(core->iq_pairs.data != NULL);

    core->center_freq = freq;
    rtlsdr_set_center_freq(core->device, core->center_freq);
}

void sdr_core_deinit(core_t *core)
{
    if (core->iq_pairs.data != NULL)
    {
        free(core->iq_pairs.data);
    }

    if (core->device_iq_pairs.data != NULL)
    {
        free(core->device_iq_pairs.data);
    }

    if (core->device != NULL)
    {
        rtlsdr_close(core->device);
    }
}

size_t sdr_audio_demodulate(buffer_float32_t *iq, buffer_float32_t *output, sdr_audio_demodulator_kind_t kind)
{
    switch (kind)
    {
    case SDR_AUDIO_DEMODULATOR_KIND_FM: {
        return demodulate_fm_float32(iq->data, iq->count, output->data, output->capacity);
    }
    break;
    case SDR_AUDIO_DEMODULATOR_KIND_AM: {
        return demodulate_am_float32(iq->data, iq->count, output->data, output->capacity);
    }
    break;
    default: {
        return 0;
    }
    break;
    }
}

void sdr_audio_pipeline_init(audio_pipeline_t *pipeline, audio_pipeline_config_t config)
{
    ASSERT(pipeline != NULL);
    pipeline->config = config;

    size_t taps_count = 8 * config.decimate_factor;
    if ((taps_count & 1) == 0)
        taps_count += 1;
    float32_t *taps = malloc(taps_count * sizeof(float32_t));
    fir_design_lowpass(taps, taps_count, 0.5f / (float32_t)config.decimate_factor, WINDOW_HAMMING);
    pipeline->decimator_fir = fir_filter_init(taps, taps_count, config.decimate_factor);
    free(taps);

    pipeline->deemphasis = biquad_filter_design_deemphasis((float32_t)config.sample_rate, 50e-6f);

    size_t n = SDR_CORE_IQ_PAIRS_DEFAULT;

    pipeline->demodulated.capacity = n;
    pipeline->demodulated.data     = malloc(n * sizeof(float32_t));
    pipeline->demodulated.count    = 0;

    pipeline->decimated.capacity = n;
    pipeline->decimated.data     = malloc(n * sizeof(float32_t));
    pipeline->decimated.count    = 0;

    pipeline->audio.capacity = n;
    pipeline->audio.data     = malloc(n * sizeof(float32_t));
    pipeline->audio.count    = 0;

    pipeline->audio_frequency.capacity = 512;
    pipeline->audio_frequency.data     = malloc(512 * sizeof(complex32_t));
    pipeline->audio_frequency.count    = 0;

    pipeline->audio_magnitude.capacity = 512;
    pipeline->audio_magnitude.data     = malloc(512 * sizeof(float32_t));
    pipeline->audio_magnitude.count    = 0;

    pipeline->audio_output.capacity = n;
    pipeline->audio_output.data     = malloc(n * sizeof(int16_t));
    pipeline->audio_output.count    = 0;

    ASSERT(pipeline->demodulated.data != NULL);
    ASSERT(pipeline->decimated.data != NULL);
    ASSERT(pipeline->audio.data != NULL);
    ASSERT(pipeline->audio_frequency.data != NULL);
    ASSERT(pipeline->audio_magnitude.data != NULL);
    ASSERT(pipeline->audio_output.data != NULL);
}

void sdr_audio_pipeline_deinit(audio_pipeline_t *pipeline)
{
    if (pipeline->demodulated.data)
    {
        free(pipeline->demodulated.data);
    }

    if (pipeline->decimated.data)
    {
        free(pipeline->decimated.data);
    }

    if (pipeline->audio.data)
    {
        free(pipeline->audio.data);
    }

    if (pipeline->audio_frequency.data)
    {
        free(pipeline->audio_frequency.data);
    }

    if (pipeline->audio_magnitude.data)
    {
        free(pipeline->audio_magnitude.data);
    }

    if (pipeline->audio_output.data)
    {
        free(pipeline->audio_output.data);
    }

    fir_filter_free(&pipeline->decimator_fir);
}

void sdr_audio_pipeline_run(core_t *core, audio_pipeline_t *pipeline)
{
    ASSERT(core != NULL);
    ASSERT(pipeline != NULL);

    audio_pipeline_config_t *config = &pipeline->config;

    // demod (FM / AM)
    pipeline->demodulated.count = sdr_audio_demodulate(&core->iq_pairs, &pipeline->demodulated, config->demodulator);

    // decimate
    switch (config->decimator)
    {
    case SDR_AUDIO_DECIMATOR_KIND_BOXCAR:
        pipeline->decimated.count = decimate_block_average_float32(
            pipeline->demodulated.data,
            pipeline->demodulated.count,
            pipeline->decimated.data,
            pipeline->decimated.capacity,
            config->decimate_factor
        );
        break;
    case SDR_AUDIO_DECIMATOR_KIND_FIR:
        pipeline->decimated.count = fir_filter_process(
            &pipeline->decimator_fir,
            pipeline->demodulated.data,
            pipeline->demodulated.count,
            pipeline->decimated.data,
            pipeline->decimated.capacity
        );
        break;
    default:
        break;
    }

    if (config->deemphasis_on)
    {
        biquad_filter_process(&pipeline->deemphasis, pipeline->decimated.data, pipeline->decimated.count);
    }

    pipeline->audio.count = float32_rads_to_float32_audio(
        pipeline->decimated.data,
        pipeline->decimated.count,
        pipeline->audio.data,
        pipeline->audio.capacity,
        core->audio_gain
    );

    pipeline->audio_output.count = float32_audio_to_int16_audio(
        pipeline->audio.data,
        pipeline->audio.count,
        pipeline->audio_output.data,
        pipeline->audio_output.capacity
    );

    size_t        fft_length = pipeline->audio_frequency.capacity;
    size_t        half       = fft_length / 2;
    window_type_t window     = config->spectrum_windowing_on ? WINDOW_HANN : WINDOW_RECTANGULAR;
    size_t        hop        = config->spectrum_averaging_on ? (fft_length / 2) : fft_length;
    size_t        available  = pipeline->audio.count;

    float32_t coherent_gain = window_coherent_gain(window, fft_length);
    float32_t scale         = 2.0f / ((float32_t)fft_length * coherent_gain);

    for (size_t i = 0; i < half; i++)
        pipeline->audio_magnitude.data[i] = 0.0f;

    size_t segments = 0;
    for (size_t start = 0; start + fft_length <= available; start += hop)
    {
        for (size_t i = 0; i < fft_length; i++)
        {
            float32_t w                                 = window_coefficient(window, i, fft_length);
            pipeline->audio_frequency.data[i].real      = pipeline->audio.data[start + i] * w;
            pipeline->audio_frequency.data[i].imaginary = 0.0f;
        }
        fft_radix2(pipeline->audio_frequency.data, fft_length);
        for (size_t i = 0; i < half; i++)
        {
            float32_t re  = pipeline->audio_frequency.data[i].real;
            float32_t im  = pipeline->audio_frequency.data[i].imaginary;
            float32_t mag = sqrtf(re * re + im * im) * scale;
            pipeline->audio_magnitude.data[i] += mag * mag;
        }
        segments++;
        if (!config->spectrum_averaging_on)
            break;
    }

    float32_t inv = (segments > 0) ? 1.0f / (float32_t)segments : 1.0f;
    for (size_t i = 0; i < half; i++)
    {
        float32_t power                   = pipeline->audio_magnitude.data[i] * inv;
        pipeline->audio_magnitude.data[i] = 10.0f * log10f(power + 1e-12f);
    }
    pipeline->audio_magnitude.count = half;
}