#include "visualization.h"
#include "raylib.h"
#include "rlImGui.h"
#include <math.h>
#include <stdlib.h>

#define PLOT_TEXTURE_WIDTH  1024
#define PLOT_TEXTURE_HEIGHT 256

typedef struct plot_internal_t
{
    RenderTexture2D texture;
} plot_internal_t;

static Color to_raylib_color(color_t c)
{
    return (Color){ .r = c.r, .g = c.g, .b = c.b, .a = c.a };
}

static float32_t read_sample(const plot_series_t *s, size_t index)
{
    if (s->type == PLOT_SAMPLE_INT16)
    {
        const int16_t *p = (const int16_t *)s->values;
        return (float32_t)p[index * s->stride];
    }
    else
    {
        const float32_t *p = (const float32_t *)s->values;
        return p[index * s->stride];
    }
}

plot_t plot_make(const char *title, float32_t y_min, float32_t y_max)
{
    plot_t plot = (plot_t){0};
    plot.title = title;
    plot.view.y_min = y_min;
    plot.view.y_max = y_max;
    plot.view.samples_per_pixel = 1.0f;
    plot.initialized = false;

    plot_internal_t *internal = (plot_internal_t *)malloc(sizeof(plot_internal_t));
    internal->texture = LoadRenderTexture(PLOT_TEXTURE_WIDTH, PLOT_TEXTURE_HEIGHT);
    plot.internal = internal;
    return plot;
}

void plot_destroy(plot_t *plot)
{
    if (plot->internal != NULL)
    {
        plot_internal_t *internal = (plot_internal_t *)plot->internal;
        UnloadRenderTexture(internal->texture);
        free(internal);
        plot->internal = NULL;
    }
}

static void draw_series(const plot_t *plot, const plot_series_t *s,
                        float32_t width, float32_t height)
{
    if (s->count < 2)
    {
        return;
    }

    const plot_view_t *view = &plot->view;

    long first = (long)floorf(view->x_start) - 1;
    long last  = (long)ceilf(view->x_start + width * view->samples_per_pixel) + 1;
    if (first < 0) { first = 0; }
    if (last > (long)s->count - 1) { last = (long)s->count - 1; }

    float32_t inv_spp = 1.0f / view->samples_per_pixel;
    float32_t y_span  = view->y_max - view->y_min;
    Color color = to_raylib_color(s->color);

    for (long i = first; i < last; ++i)
    {
        float32_t v0 = read_sample(s, (size_t)i);
        float32_t v1 = read_sample(s, (size_t)(i + 1));

        float32_t x0 = ((float32_t)i       - view->x_start) * inv_spp;
        float32_t x1 = ((float32_t)(i + 1) - view->x_start) * inv_spp;
        float32_t y0 = height - ((v0 - view->y_min) / y_span) * height;
        float32_t y1 = height - ((v1 - view->y_min) / y_span) * height;

        DrawLine((int)x0, (int)y0, (int)x1, (int)y1, color);
    }
}

void plot_render(plot_t *plot, const plot_series_t *series, size_t series_count)
{
    ASSERT(plot != NULL);
    plot_internal_t *internal = (plot_internal_t *)plot->internal;

    float32_t width  = (float32_t)PLOT_TEXTURE_WIDTH;
    float32_t height = (float32_t)PLOT_TEXTURE_HEIGHT;

    size_t max_count = 0;
    for (size_t i = 0; i < series_count; ++i)
    {
        if (series[i].count > max_count) { max_count = series[i].count; }
    }
    if (!plot->initialized && max_count >= 2)
    {
        plot->view.x_start = 0.0f;
        plot->view.samples_per_pixel = (float32_t)max_count / width;
        if (plot->view.samples_per_pixel <= 0.0f) { plot->view.samples_per_pixel = 1.0f; }
        plot->initialized = true;
    }

    BeginTextureMode(internal->texture);
        ClearBackground((Color){10, 10, 12, 255});
        for (size_t i = 0; i < series_count; ++i)
        {
            draw_series(plot, &series[i], width, height);
        }
    EndTextureMode();
}

void plot_to_gui(plot_t *plot)
{
    ASSERT(plot != NULL);
    plot_internal_t *internal = (plot_internal_t *)plot->internal;
    rlImGuiImageRenderTexture(&internal->texture);
}