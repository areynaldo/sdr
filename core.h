#ifndef SDR_CORE_H
#define SDR_CORE_H

#include "base.h"

#include "dsp.h"
#include "rtl-sdr.h"

#ifndef KHz
#define KHz(x) x * 1000.0
#endif

#ifndef MHz
#define MHz(x) KHz(x) * 1000.0
#endif

#ifndef GHz
#define GHz(x) MHz(x) * 1000.0
#endif

#ifndef SDR_FM_BAND_START
#define SDR_FM_BAND_START 87500000.0
#endif

#ifndef SDR_FM_BAND_END
#define SDR_FM_BAND_END 108000000.0
#endif
#define SDR_FM_BAND_WIDTH (SDR_FM_BAND_END - SDR_FM_BAND_START)

#define SDR_CORE_AUDIO_GAIN_DEFAULT 8000.0
#define SDR_CORE_CENTER_FREQ_DEFAULT 97300000 // Bayern3
#define SDR_CORE_SAMPLE_RATE_DEFAULT 250000
#define SDR_CORE_IQ_PAIRS_DEFAULT 4096

typedef enum core_error_t
{
    CORE_ERROR_NONE = 0,
    CORE_ERROR_READ_SYNC_FAILED,
    CORE_ERROR_COUNT
} core_error_t;

static const char *core_error_strings[CORE_ERROR_COUNT] = {
    "none",
    "read sync failed",
};

typedef struct core_t
{
    // settings
    float32_t audio_gain;
    uint32_t sample_rate;
    float32_t center_freq;
    uint32_t iq_pairs_count;

    // device
    rtlsdr_dev_t *device;
    uint8_t *device_iq_pairs_buffer;

    // iq pairs
    float32_t *iq_pairs_buffer;
    size_t iq_pairs_buffer_capacity;
    size_t iq_pairs_buffer_count;

    // fm
    uint32_t decimate_factor;
    float32_t *demodulated_buffer;
    size_t demodulated_buffer_capacity;
    size_t demodulated_buffer_count;

    // audio
    uint32_t audio_sample_rate;
    uint32_t audio_sample_size;
    uint32_t audio_channels;

    void *decimated_buffer;
    size_t decimated_buffer_capacity;
    size_t decimated_buffer_count;

    uint8_t *audio_buffer;
    size_t audio_buffer_capacity;
    size_t audio_buffer_count;
} core_t;

// core
core_t core_init(core_t settings);
void core_deinit(core_t *core);

// iq pairs
core_error_t core_read_iq_pairs_sync(core_t *core);

// radio
void core_set_center_freq(core_t *core, float32_t freq);

// fm
void setup_fm_pipeline(core_t *core,
                       uint32_t decimate_factor,
                       uint32_t sample_rate,
                       size_t sample_size,
                       uint32_t channels);

#endif