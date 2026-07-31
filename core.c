#include "core.h"

core_t core_init(
    core_t settings)
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

    if (settings.iq_pairs_buffer_capacity == 0)
    {
        settings.iq_pairs_buffer_capacity = 2 * settings.iq_pairs_count;
    }

    if (settings.iq_pairs_buffer_count == 0)
    {
        settings.iq_pairs_buffer_count = 0;
    }

    if (settings.iq_pairs_buffer == NULL)
    {
        settings.device_iq_pairs_buffer = (uint8_t *)malloc(settings.iq_pairs_buffer_capacity * sizeof(uint8_t));
        if (settings.device_iq_pairs_buffer == NULL)
        {
            printf("Failed to allocate iq pairs buffer.\n");
            return settings;
        }
        settings.iq_pairs_buffer = (float32_t *)malloc(settings.iq_pairs_buffer_capacity * sizeof(float32_t));
        if (settings.iq_pairs_buffer == NULL)
        {
            printf("Failed to allocate iq pairs buffer.\n");
            return settings;
        }
    }

    if (settings.device == 0)
    {
        if (rtlsdr_open(&settings.device, 0) < 0)
        {
            printf("Failed to open device 0 (in use by another app, or driver issue).\n");
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

core_error_t core_read_iq_pairs_sync(core_t *core)
{
    ASSERT(core != NULL);
    ASSERT(core->device != NULL);
    ASSERT(core->device_iq_pairs_buffer != NULL);
    ASSERT(core->iq_pairs_buffer != NULL);

    int read_count = 0;
    core->iq_pairs_buffer_count = 0;
    if (rtlsdr_read_sync(core->device, core->device_iq_pairs_buffer, core->iq_pairs_buffer_capacity, &read_count) < 0 || read_count == 0)
    {
        return CORE_ERROR_READ_SYNC_FAILED;
    }

    core->iq_pairs_buffer_count = (size_t)read_count;
    uint8_buffer_to_float32_buffer(core->device_iq_pairs_buffer, core->iq_pairs_buffer_count, core->iq_pairs_buffer, core->iq_pairs_buffer_count);
    return CORE_ERROR_NONE;
}

void core_set_center_freq(core_t *core, float32_t freq)
{
    ASSERT(core != NULL);
    ASSERT(core->device != NULL);
    ASSERT(core->device_iq_pairs_buffer != NULL);
    ASSERT(core->iq_pairs_buffer != NULL);

    core->center_freq = freq;
    rtlsdr_set_center_freq(core->device, core->center_freq);
}

void core_deinit(core_t *core)
{
    if (core->iq_pairs_buffer != NULL)
    {
        free(core->iq_pairs_buffer);
    }

    if (core->device_iq_pairs_buffer != NULL)
    {
        free(core->device_iq_pairs_buffer);
    }

    if (core->demodulated_buffer != NULL)
    {
        free(core->demodulated_buffer);
    }

    if (core->decimated_buffer != NULL)
    {
        free(core->decimated_buffer);
    }

    if (core->audio_buffer != NULL)
    {
        free(core->audio_buffer);
    }

    if (core->device != NULL)
    {
        rtlsdr_close(core->device);
    }
}

void setup_fm_pipeline(core_t *core,
                       uint32_t decimate_factor,
                       uint32_t sample_rate,
                       size_t sample_size,
                       uint32_t channels)
{
    ASSERT(core != NULL);

    core->decimate_factor = decimate_factor;
    core->audio_sample_size = sample_size;

    core->demodulated_buffer_count = 0;
    if (core->demodulated_buffer == NULL)
    {
        core->demodulated_buffer_capacity = core->iq_pairs_count;
        core->demodulated_buffer = malloc(core->demodulated_buffer_capacity * sizeof(float32_t));
    }

    core->decimated_buffer_count = 0;
    if (core->decimated_buffer == NULL)
    {
        core->decimated_buffer_capacity = core->iq_pairs_count;
        core->decimated_buffer = malloc(core->decimated_buffer_capacity * sizeof(float32_t));
    }

    core->audio_buffer_count = 0;
    if (core->audio_buffer == NULL)
    {
        core->audio_buffer_capacity = core->iq_pairs_count;
        core->audio_buffer = malloc(core->audio_buffer_capacity * core->audio_sample_size);
    }

    ASSERT(core->decimated_buffer != NULL);
    ASSERT(core->audio_buffer != NULL);
}

void run_fm_pipeline(core_t *core)
{
    ASSERT(core != NULL);

    core->demodulated_buffer_count = demodulate_fm_float32(core->iq_pairs_buffer,
                                                           core->iq_pairs_buffer_count,
                                                           core->demodulated_buffer,
                                                           core->demodulated_buffer_capacity);

    core->decimated_buffer_count = decimate_block_average_float32(core->demodulated_buffer,
                                                                  core->demodulated_buffer_count,
                                                                  core->decimated_buffer,
                                                                  core->decimated_buffer_capacity,
                                                                  core->decimate_factor);

    core->audio_buffer_count = float32_rads_to_int16_audio(core->decimated_buffer,
                                                           core->decimated_buffer_count,
                                                           (int16_t *)core->audio_buffer,
                                                           core->audio_buffer_capacity, core->audio_gain);
}