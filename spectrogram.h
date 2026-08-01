#ifndef SDR_SPECTROGRAM_H
#define SDR_SPECTROGRAM_H

#include "base.h"

typedef struct spectrogram_t
{
    const char *title;
    float32_t magnitude_min;
    float32_t magnitude_max;
    size_t history_width;
    size_t frequency_height;
    size_t write_column;
    void *internal;
} spectrogram_t;

spectrogram_t spectrogram_make(const char *title,
                               float32_t magnitude_min, float32_t magnitude_max,
                               size_t history_width, size_t frequency_height);
void spectrogram_destroy(spectrogram_t *spectrogram);

void spectrogram_render(spectrogram_t *spectrogram,
                        const float32_t *magnitudes, size_t magnitudes_count);

void spectrogram_to_gui(spectrogram_t *spectrogram);

#endif