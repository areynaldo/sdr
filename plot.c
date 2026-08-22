#include <stdlib.h>

#include "plot.h"

static inline uint8_t plot_lerp_u8(uint8_t a, uint8_t b, float32_t t)
{
    return (uint8_t)((float32_t)a + ((float32_t)b - (float32_t)a) * t);
}

static color_t plot_colormap_sdr(float32_t t)
{
    color_t stops[5] = {
        { 48, 128, 255, 255}, // blue
        { 48, 255, 128, 255}, // green
        {255, 240,  64, 255}, // yellow
        {255, 144,  32, 255}, // orange
        {255,  48,  48, 255}, // red
    };
    const size_t segment_count = 4;

    float32_t scaled = t * (float32_t)segment_count;
    size_t    index  = (size_t)scaled;
    if (index >= segment_count)
    {
        index = segment_count - 1; // clamp t == 1.0
    }
    float32_t fraction = scaled - (float32_t)index;

    color_t a = stops[index];
    color_t b = stops[index + 1];
    return (color_t){
        .r = plot_lerp_u8(a.r, b.r, fraction),
        .g = plot_lerp_u8(a.g, b.g, fraction),
        .b = plot_lerp_u8(a.b, b.b, fraction),
        .a = 255,
    };
}

void plot_draw_line(vector2_t start_point, vector2_t end_point, float32_t thickness, color_t color)
{
    ASSERT(thickness >= 0.0f);

    Vector2 rl_start_point = {.x = start_point.x, .y = start_point.y};
    Vector2 rl_end_point   = {.x = end_point.x, .y = end_point.y};
    Color   rl_color       = {.r = color.r, .g = color.g, .b = color.b, .a = color.a};
    DrawLineEx(rl_start_point, rl_end_point, thickness, rl_color);
}

static float32_t plot_value_to_y(rectangle_t target, plot_t params, float32_t scaling, float32_t value)
{
    float32_t y = target.y + (params.max - value) * scaling;
    if (y < target.y)
    {
        y = target.y;
    }
    if (y > target.y + target.height)
    {
        y = target.y + target.height;
    }
    return y;
}

static void plot_curve(rectangle_t target, plot_t params)
{
    ASSERT(params.data != NULL);
    ASSERT(params.data_count > 0);
    ASSERT(params.max > params.min);

    float32_t values_range      = params.max - params.min;
    float32_t scaling           = target.height / values_range;
    size_t    width             = (size_t)target.width;
    float32_t samples_per_pixel = (float32_t)params.data_count / (float32_t)width;

    if (samples_per_pixel > 1.0f)
    {
        // More samples than pixels: draw a vertical min/max bar per column.
        for (size_t x = 0; x < width; x++)
        {
            size_t start = (size_t)((float32_t)x * samples_per_pixel);
            size_t end   = (size_t)((float32_t)(x + 1) * samples_per_pixel);
            if (end > params.data_count)
            {
                end = params.data_count;
            }
            if (start >= end)
            {
                continue;
            }

            float32_t min_value = params.data[start];
            float32_t max_value = params.data[start];
            for (size_t i = start + 1; i < end; i++)
            {
                float32_t value = params.data[i];
                min_value       = value < min_value ? value : min_value;
                max_value       = value > max_value ? value : max_value;
            }

            float32_t pixel_x   = target.x + (float32_t)x;
            vector2_t min_point = {.x = pixel_x, .y = plot_value_to_y(target, params, scaling, max_value)};
            vector2_t max_point = {.x = pixel_x, .y = plot_value_to_y(target, params, scaling, min_value)};
            plot_draw_line(min_point, max_point, params.size, params.color);
        }
    }
    else
    {
        // Fewer samples than pixels: connect consecutive points.
        float32_t x_step =
            (params.data_count > 1) ? (target.width - 1.0f) / (float32_t)(params.data_count - 1) : 0.0f;

        vector2_t previous = {
            .x = target.x,
            .y = plot_value_to_y(target, params, scaling, params.data[0]),
        };
        for (size_t i = 1; i < params.data_count; i++)
        {
            vector2_t current = {
                .x = target.x + (float32_t)i * x_step,
                .y = plot_value_to_y(target, params, scaling, params.data[i]),
            };
            plot_draw_line(previous, current, params.size, params.color);
            previous = current;
        }
    }
}

static void plot_scatter(rectangle_t target, plot_t params)
{
    ASSERT(params.data != NULL);
    ASSERT(params.data_count > 0);
    ASSERT(target.width > 0.0f);
    ASSERT(target.height > 0.0f);
    ASSERT(params.max > params.min);
    ASSERT(params.size > 0.0f);

    float32_t values_range = params.max - params.min;
    float32_t scaling      = target.height / values_range;
    float32_t x_step       =
        (params.data_count > 1) ? (target.width - 1.0f) / (float32_t)(params.data_count - 1) : 0.0f;

    Color rl_color = {.r = params.color.r, .g = params.color.g, .b = params.color.b, .a = params.color.a};
    for (size_t i = 0; i < params.data_count; i++)
    {
        float32_t pixel_x = target.x + (float32_t)i * x_step;
        float32_t pixel_y = plot_value_to_y(target, params, scaling, params.data[i]);
        DrawCircleV((Vector2){pixel_x, pixel_y}, params.size, rl_color);
    }
}

static void plot_heatmap_init(plot_heatmap_t *heatmap, size_t bins)
{
    heatmap->bins         = bins;
    heatmap->history      = heatmap->history > 0 ? heatmap->history : 512;
    heatmap->write_column = 0;
    heatmap->pixels       = calloc(bins * heatmap->history, sizeof(color_t));
    ASSERT(heatmap->pixels != NULL);

    Image image      = GenImageColor((int)heatmap->history, (int)bins, BLACK);
    heatmap->texture = LoadTextureFromImage(image);
    UnloadImage(image);

    heatmap->initialized = true;
}

static void plot_heatmap(rectangle_t target, plot_t params)
{
    ASSERT(params.data != NULL);
    ASSERT(params.heatmap != NULL);
    ASSERT(params.max > params.min);

    plot_heatmap_t *heatmap = params.heatmap;
    if (!heatmap->initialized)
    {
        plot_heatmap_init(heatmap, params.data_count);
    }

    float32_t range = params.max - params.min;
    for (size_t bin = 0; bin < heatmap->bins && bin < params.data_count; bin++)
    {
        float32_t t = (params.data[bin] - params.min) / range;
        t           = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

        size_t row              = heatmap->bins - 1 - bin; // low frequency at the bottom
        size_t offset           = row * heatmap->history + heatmap->write_column;
        heatmap->pixels[offset] = plot_colormap_sdr(t);
    }
    UpdateTexture(heatmap->texture, heatmap->pixels);
    heatmap->write_column = (heatmap->write_column + 1) % heatmap->history;

    // Draw the ring buffer with the newest column on the right.
    float32_t split      = (float32_t)heatmap->write_column;
    float32_t width_left = target.width * ((float32_t)heatmap->history - split) / (float32_t)heatmap->history;

    Rectangle source_left = {split, 0.0f, (float32_t)heatmap->history - split, (float32_t)heatmap->bins};
    DrawTexturePro(
        heatmap->texture,
        source_left,
        (Rectangle){target.x, target.y, width_left, target.height},
        (Vector2){0, 0},
        0,
        WHITE
    );

    Rectangle source_right = {0.0f, 0.0f, split, (float32_t)heatmap->bins};
    DrawTexturePro(
        heatmap->texture,
        source_right,
        (Rectangle){target.x + width_left, target.y, target.width - width_left, target.height},
        (Vector2){0, 0},
        0,
        WHITE
    );
}

void plot_heatmap_free(plot_heatmap_t *heatmap)
{
    if (heatmap == NULL || !heatmap->initialized)
    {
        return;
    }
    UnloadTexture(heatmap->texture);
    free(heatmap->pixels);
    heatmap->pixels      = NULL;
    heatmap->initialized = false;
}

void plot(rectangle_t target, plot_t params)
{
    switch (params.type)
    {
    case PLOT_TYPE_CURVE:
        plot_curve(target, params);
        break;
    case PLOT_TYPE_SCATTER:
        plot_scatter(target, params);
        break;
    case PLOT_TYPE_HEATMAP:
        plot_heatmap(target, params);
        break;
    default:
        break;
    }
}