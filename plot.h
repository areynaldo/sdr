#ifndef SDR_PLOT_H
#define SDR_PLOT_H

#include "base.h"
#include "raylib.h"

typedef enum plot_type_t
{
    PLOT_TYPE_NONE,
    PLOT_TYPE_CURVE,
    PLOT_TYPE_SCATTER,
    PLOT_TYPE_HEATMAP,
    PLOT_TYPE_COUNT,
} plot_type_t;

typedef struct plot_heatmap_t
{
    Texture2D texture;
    color_t  *pixels;
    size_t    bins;
    size_t    history;
    size_t    write_column;
    bool      initialized;
} plot_heatmap_t;

typedef struct plot_t
{
    plot_type_t     type;
    float32_t      *data;
    size_t          data_count;
    float32_t       min;
    float32_t       max;
    color_t         color;
    float32_t       size;
    plot_heatmap_t *heatmap;
} plot_t;

void plot(rectangle_t target, plot_t params);
void plot_heatmap_free(plot_heatmap_t *heatmap);
void plot_draw_line(vector2_t start_point, vector2_t end_point, float32_t thickness, color_t color);

#endif // SDR_PLOT_H