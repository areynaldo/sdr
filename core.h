#ifndef SDR_CORE_H
#define SDR_CORE_H

#include "base.h"
#include "dsp.h"
#include "fft.h"
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

#define SDR_CORE_AUDIO_GAIN_DEFAULT  0.318    // approx. (volume / PI)
#define SDR_CORE_CENTER_FREQ_DEFAULT 97300000 // Bayern3
#define SDR_CORE_SAMPLE_RATE_DEFAULT 250000
#define SDR_CORE_IQ_PAIRS_DEFAULT    4096

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

// audio pipleine

typedef enum sdr_audio_demodulator_kind_t
{
    SDR_AUDIO_DEMODULATOR_KIND_FM,
    SDR_AUDIO_DEMODULATOR_KIND_AM,
    SDR_AUDIO_DEMODULATOR_KIND_COUNT
} sdr_audio_demodulator_kind_t;

static char *SDR_AUDIO_DEMODULATOR_KIND_STRINGS[SDR_AUDIO_DEMODULATOR_KIND_COUNT] = {
    "FM",
    "AM",
};

typedef enum sdr_audio_decimator_kind_t
{
    SDR_AUDIO_DECIMATOR_KIND_BOXCAR,
    SDR_AUDIO_DECIMATOR_KIND_FIR,
    SDR_AUDIO_DECIMATOR_KIND_COUNT,
} sdr_audio_decimator_kind_t;

static char *SDR_AUDIO_DECIMATOR_KIND_STRINGS[SDR_AUDIO_DECIMATOR_KIND_COUNT] = {
    "BOXCAR",
    "FIR",
};

typedef struct audio_pipeline_config_t
{
    sdr_audio_demodulator_kind_t  demodulator;
    sdr_audio_decimator_kind_t    decimator;
    bool                          deemphasis_on;
    bool                          spectrum_windowing_on;
    bool                          spectrum_averaging_on;
    uint32_t                      decimate_factor;
    uint32_t                      sample_rate;
    uint32_t                      sample_size;
    uint32_t                      channels;
} audio_pipeline_config_t;

typedef struct audio_pipeline_t
{
    audio_pipeline_config_t config;

    buffer_float32_t   demodulated;
    buffer_float32_t   decimated;
    buffer_float32_t   audio;
    buffer_complex32_t audio_frequency;
    buffer_float32_t   audio_magnitude;
    buffer_int16_t     audio_output;
    fir_filter_t       decimator_fir;
    biquad_filter_t    deemphasis;
} audio_pipeline_t;

// core
typedef struct core_t
{
    // settings
    float32_t audio_gain;
    uint32_t  sample_rate;
    float32_t center_freq;
    uint32_t  iq_pairs_count;

    // device
    rtlsdr_dev_t  *device;
    buffer_uint8_t device_iq_pairs;

    // iq pairs
    buffer_float32_t iq_pairs;
} core_t;

// core
core_t sdr_core_init(core_t settings);
void sdr_core_deinit(core_t *core);

// iq pairs
core_error_t sdr_core_read_iq_pairs_sync(core_t *core);

// radio
void sdr_core_set_center_freq(core_t *core, float32_t freq);

// audio
void sdr_audio_pipeline_init(audio_pipeline_t *pipeline, audio_pipeline_config_t config);
void sdr_audio_pipeline_deinit(audio_pipeline_t *pipeline);
void sdr_audio_pipeline_run(core_t *core, audio_pipeline_t *pipeline);

#endif