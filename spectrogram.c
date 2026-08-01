#include "spectrogram.h"
#include "raylib.h"
#include "rlImGui.h"
#include <stdlib.h>

typedef struct spectrogram_internal_t
{
    RenderTexture2D texture;
    Color *pixels;
} spectrogram_internal_t;

static Color magnitude_to_color(float32_t normalized)
{
    if (normalized < 0.0f)
    {
        normalized = 0.0f;
    }
    if (normalized > 1.0f)
    {
        normalized = 1.0f;
    }

    float32_t r;
    float32_t g;
    float32_t b;
    if (normalized < 0.33f)
    {
        float32_t t = normalized / 0.33f;
        r = 0.0f;
        g = t;
        b = 1.0f;
    }
    else if (normalized < 0.66f)
    {
        float32_t t = (normalized - 0.33f) / 0.33f;
        r = t;
        g = 1.0f;
        b = 1.0f - t;
    }
    else
    {
        float32_t t = (normalized - 0.66f) / 0.34f;
        r = 1.0f;
        g = 1.0f - t;
        b = 0.0f;
    }

    Color color;
    color.r = (uint8_t)(r * 255.0f);
    color.g = (uint8_t)(g * 255.0f);
    color.b = (uint8_t)(b * 255.0f);
    color.a = 255;
    return color;
}

spectrogram_t spectrogram_make(const char *title,
                               float32_t magnitude_min, float32_t magnitude_max,
                               size_t history_width, size_t frequency_height)
{
    spectrogram_t spectrogram = (spectrogram_t){0};
    spectrogram.title = title;
    spectrogram.magnitude_min = magnitude_min;
    spectrogram.magnitude_max = magnitude_max;
    spectrogram.history_width = history_width;
    spectrogram.frequency_height = frequency_height;
    spectrogram.write_column = 0;

    spectrogram_internal_t *internal =
        (spectrogram_internal_t *)malloc(sizeof(spectrogram_internal_t));
    internal->texture = LoadRenderTexture((int)history_width, (int)frequency_height);
    internal->pixels = (Color *)malloc(history_width * frequency_height * sizeof(Color));

    BeginTextureMode(internal->texture);
    ClearBackground(magnitude_to_color(0.0f));
    EndTextureMode();

    for (size_t i = 0; i < history_width * frequency_height; ++i)
    {
        internal->pixels[i] = magnitude_to_color(0.0f);
    }

    UpdateTextureRec(internal->texture.texture,
                     (Rectangle){0.0f, 0.0f, (float)history_width, (float)frequency_height},
                     internal->pixels);

    spectrogram.internal = internal;
    return spectrogram;
}

void spectrogram_destroy(spectrogram_t *spectrogram)
{
    if (spectrogram->internal != NULL)
    {
        spectrogram_internal_t *internal =
            (spectrogram_internal_t *)spectrogram->internal;
        UnloadRenderTexture(internal->texture);
        free(internal->pixels);
        free(internal);
        spectrogram->internal = NULL;
    }
}

void spectrogram_render(spectrogram_t *spectrogram,
                        const float32_t *magnitudes, size_t magnitudes_count)
{
    ASSERT(spectrogram != NULL);
    ASSERT(magnitudes != NULL);

    spectrogram_internal_t *internal =
        (spectrogram_internal_t *)spectrogram->internal;

    size_t height = spectrogram->frequency_height;
    size_t width = spectrogram->history_width;
    size_t rows = (magnitudes_count < height) ? magnitudes_count : height;
    float32_t span = spectrogram->magnitude_max - spectrogram->magnitude_min;
    if (span <= 0.0f)
    {
        span = 1.0f;
    }

    if (width < 2)
    {
        width = 1;
    }

    for (size_t row = 0; row < height; ++row)
    {
        size_t flipped_row = height - 1 - row;
        Color *row_pixels = &internal->pixels[flipped_row * spectrogram->history_width];

        if (spectrogram->history_width > 1)
        {
            memmove(row_pixels,
                    row_pixels + 1,
                    (spectrogram->history_width - 1) * sizeof(Color));
        }

        float32_t value_db = (row < rows) ? magnitudes[row] : spectrogram->magnitude_min;
        float32_t normalized = (value_db - spectrogram->magnitude_min) / span;
        row_pixels[spectrogram->history_width - 1] = magnitude_to_color(normalized);
    }

    UpdateTextureRec(internal->texture.texture,
                     (Rectangle){0.0f, 0.0f, (float)spectrogram->history_width, (float)height},
                     internal->pixels);

    spectrogram->write_column = spectrogram->history_width - 1;
}

void spectrogram_to_gui(spectrogram_t *spectrogram)
{
    ASSERT(spectrogram != NULL);
    spectrogram_internal_t *internal =
        (spectrogram_internal_t *)spectrogram->internal;
    rlImGuiImageRenderTexture(&internal->texture);
}