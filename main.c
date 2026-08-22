#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "cimgui.h"
#include "rlImGui.h"

#include "core.c"
#include "dsp.c"
#include "fft.c"
#include "plot.c"

#define UI_FONT_SIZE 20

static void buffer_min_max(float32_t *data, size_t count, float32_t *out_min, float32_t *out_max)
{
    float32_t lo = data[0];
    float32_t hi = data[0];
    for (size_t i = 1; i < count; i++)
    {
        lo = data[i] < lo ? data[i] : lo;
        hi = data[i] > hi ? data[i] : hi;
    }
    *out_min = lo;
    *out_max = hi + 1e-6f; // avoid max == min
}

static void ensure_render_texture(RenderTexture2D *render_texture, int width, int height)
{
    if (width < 1)
    {
        width = 1;
    }
    if (height < 1)
    {
        height = 1;
    }
    if (render_texture->texture.width == width && render_texture->texture.height == height)
    {
        return;
    }
    if (render_texture->id != 0)
    {
        UnloadRenderTexture(*render_texture);
    }
    *render_texture = LoadRenderTexture(width, height);
}

int main(void)
{
    // window
    const int window_width  = 1280;
    const int window_height = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "SDR");
    SetTargetFPS(60);

    // font
    Font fonts[1];
    fonts[0] = LoadFontEx("fonts/IBMPlexMono-Regular.ttf", UI_FONT_SIZE, 0, 0);
    SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_POINT);

    // gui
    rlImGuiSetup(true);
    ImGuiIO *io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // core
    core_t           core           = sdr_core_init((core_t){0});
    audio_pipeline_t audio_pipeline = {0};
    sdr_audio_pipeline_init(
        &audio_pipeline,
        (audio_pipeline_config_t){
            .demodulator           = SDR_AUDIO_DEMODULATOR_KIND_FM,
            .decimator             = SDR_AUDIO_DECIMATOR_KIND_FIR,
            .deemphasis_on         = true,
            .spectrum_windowing_on = true,
            .spectrum_averaging_on = true,
            .decimate_factor       = 5,
            .sample_rate           = 50000,
            .sample_size           = sizeof(int16_t),
            .channels              = 1
        }
    );

    // audio init
    SetAudioStreamBufferSizeDefault(core.iq_pairs_count / audio_pipeline.config.decimate_factor);
    InitAudioDevice();
    AudioStream audio_stream = LoadAudioStream(audio_pipeline.config.sample_rate, 16, audio_pipeline.config.channels);
    PlayAudioStream(audio_stream);

    // plots
    plot_heatmap_t  waterfall    = {.history = 512};
    RenderTexture2D audio_rt  = {0};
    RenderTexture2D spectrum_rt  = {0};
    RenderTexture2D waterfall_rt = {0};

    // main loop
    while (!WindowShouldClose())
    {
        sdr_core_read_iq_pairs_sync(&core);
        sdr_audio_pipeline_run(&core, &audio_pipeline);
        float32_t audio_magnitude_min;
        float32_t audio_magnitude_max;
        buffer_min_max(
            audio_pipeline.audio_magnitude.data,
            audio_pipeline.audio_magnitude.count,
            &audio_magnitude_min,
            &audio_magnitude_max
        );

        if (IsAudioStreamProcessed(audio_stream))
        {
            UpdateAudioStream(audio_stream, audio_pipeline.audio_output.data, audio_pipeline.audio_output.count);
        }

        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);
        rlImGuiBegin();

        if (igBeginMainMenuBar())
        {
            if (igBeginMenu("File", true))
            {
                if (igMenuItem_Bool("Quit", "Esc", false, true))
                {
                    // WindowShouldClose() already handles Esc; set a flag if you want a menu quit
                }
                igEndMenu();
            }
            if (igBeginMenu("View", true))
            {
                igMenuItem_Bool("Spectrum", NULL, false, true);
                igMenuItem_Bool("Waterfall", NULL, false, true);
                igMenuItem_Bool("Audio", NULL, false, true);
                igEndMenu();
            }
            igEndMainMenuBar();
        }

        igDockSpaceOverViewport(0, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);

        igBegin("OPTIONS", NULL, 0);
        {
            igSeparatorText("RADIO");

            float32_t center_mhz = core.center_freq / 1000000.0f;
            if (igSliderFloat("center (MHz)", &center_mhz, 87.5f, 108.0f, "%.1f", 0))
            {
                sdr_core_set_center_freq(&core, center_mhz * 1000000.0f);
            }

            igSeparatorText("AUDIO");

            igSliderFloat("gain", &core.audio_gain, 0.0f, 1.0f, "%.3f", 0);

            igCombo_Str_arr(
                "demodulator",
                (int *)&audio_pipeline.config.demodulator,
                SDR_AUDIO_DEMODULATOR_KIND_STRINGS,
                SDR_AUDIO_DEMODULATOR_KIND_COUNT,
                -1
            );

            igCheckbox("deemphasis", &audio_pipeline.config.deemphasis_on);
            igCheckbox("spectrum windowing", &audio_pipeline.config.spectrum_windowing_on);
            igCheckbox("spectrum averaging", &audio_pipeline.config.spectrum_averaging_on);

            igSeparatorText("PIPELINE (init-time)");

            igBeginDisabled(true);
            igCombo_Str_arr(
                "decimator",
                (int *)&audio_pipeline.config.decimator,
                SDR_AUDIO_DECIMATOR_KIND_STRINGS,
                SDR_AUDIO_DECIMATOR_KIND_COUNT,
                -1
            );
            int decimate_factor = (int)audio_pipeline.config.decimate_factor;
            igInputInt("decimate factor", &decimate_factor, 1, 1, 0);
            int sample_rate = (int)audio_pipeline.config.sample_rate;
            igInputInt("sample rate", &sample_rate, 1000, 10000, 0);
            igEndDisabled();
        }
        igEnd();

        if (igBegin("AUDIO", NULL, 0))
        {
            int width  = (int)igGetWindowWidth();
            int height = (int)igGetWindowHeight();
            ensure_render_texture(&audio_rt, width, height);

            BeginTextureMode(audio_rt);
            ClearBackground(BLACK);
            if (audio_pipeline.audio.count > 0)
            {
                plot(
                    (rectangle_t){0, 0, (float32_t)width, (float32_t)height},
                    (plot_t){
                        .type       = PLOT_TYPE_CURVE,
                        .data       = audio_pipeline.audio.data,
                        .data_count = audio_pipeline.audio.count,
                        .min        = -1.0f,
                        .max        = 1.0f,
                        .color      = (color_t){255, 144, 32, 255},
                        .size       = 1.0f,
                    }
                );
            }
            EndTextureMode();

            rlImGuiImageRenderTextureFit(&audio_rt, true);
        }
        igEnd();

        if (igBegin("SPECTRUM", NULL, 0))
        {
            int width  = (int)igGetWindowWidth();
            int height = (int)igGetWindowHeight();
            ensure_render_texture(&spectrum_rt, width, height);

            BeginTextureMode(spectrum_rt);
            ClearBackground(BLACK);
            plot(
                (rectangle_t){0, 0, (float32_t)width, (float32_t)height},
                (plot_t){
                    .type       = PLOT_TYPE_CURVE,
                    .data       = audio_pipeline.audio_magnitude.data,
                    .data_count = audio_pipeline.audio_magnitude.count,
                    .min        = audio_magnitude_min,
                    .max        = audio_magnitude_max,
                    .color      = (color_t){48, 255, 128, 255},
                    .size       = 1.5f,
                }
            );
            EndTextureMode();

            rlImGuiImageRenderTextureFit(&spectrum_rt, true);
        }
        igEnd();

        if (igBegin("WATERFALL", NULL, 0))
        {
            int width  = (int)igGetWindowWidth();
            int height = (int)igGetWindowHeight();
            ensure_render_texture(&waterfall_rt, width, height);

            BeginTextureMode(waterfall_rt);
            ClearBackground(BLACK);
            plot(
                (rectangle_t){0, 0, (float32_t)width, (float32_t)height},
                (plot_t){
                    .type       = PLOT_TYPE_HEATMAP,
                    .data       = audio_pipeline.audio_magnitude.data,
                    .data_count = audio_pipeline.audio_magnitude.count,
                    .min        = audio_magnitude_min,
                    .max        = audio_magnitude_max,
                    .heatmap    = &waterfall,
                }
            );
            EndTextureMode();

            rlImGuiImageRenderTextureFit(&waterfall_rt, true);
        }
        igEnd();

        rlImGuiEnd();
        EndDrawing();
    }

    // cleanup
    plot_heatmap_free(&waterfall);
    UnloadRenderTexture(spectrum_rt);
    UnloadRenderTexture(waterfall_rt);

    sdr_audio_pipeline_deinit(&audio_pipeline);
    sdr_core_deinit(&core);
    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    UnloadFont(fonts[0]);
    CloseWindow();
    return 0;
}