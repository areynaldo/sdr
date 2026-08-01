#ifndef SDR_VISUALIZATION_H
#define SDR_VISUALIZATION_H

#include "base.h"

typedef enum plot_sample_type_t
{
    PLOT_SAMPLE_FLOAT32,
    PLOT_SAMPLE_INT16,
} plot_sample_type_t;

typedef struct plot_series_t
{
    const void *values;
    size_t count;
    size_t stride;
    plot_sample_type_t type;
    color_t color;
} plot_series_t;

typedef struct plot_view_t
{
    float32_t y_min;
    float32_t y_max;
    float32_t x_start;
    float32_t samples_per_pixel;
} plot_view_t;

typedef struct plot_t
{
    const char *title;
    plot_view_t view;
    bool initialized;
    void *internal;
} plot_t;

plot_t plot_make(const char *title, float32_t y_min, float32_t y_max);
void plot_destroy(plot_t *plot);

// raylib renders series into the plot's texture. call before rlImGuiBegin.
void plot_render(plot_t *plot, const plot_series_t *series, size_t series_count);

// blits the plot's texture as an imgui image. call inside an igBegin/igEnd window.
void plot_to_gui(plot_t *plot);

#endif