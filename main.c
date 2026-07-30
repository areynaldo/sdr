#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "base.h"
#include "sdr.h"
#include "sdr.c"
#include "rtl-sdr.h"
#include "core.h"
#include "core.c"
#include "gui.h"
#include "gui.c"

#include "raylib.h"
#include "rlImGui.h"

static inline float lane_y(float v, float vmin, float vmax, float top, float bot) {
    float t = (v - vmin) / (vmax - vmin);
    return bot - t * (bot - top);
}

int main(void)
{
    core_t core = core_init((core_t){0});

    float *demodulated_buffer = (float *)malloc(core.iq_pairs * sizeof(float));
    uint32_t decimate_factor = 5;
    size_t audio_buffer_count = (core.iq_pairs / decimate_factor + 1) * sizeof(int16_t);
    int16_t *audio_buffer = (int16_t *)malloc(audio_buffer_count);
    float32_t *decimated_buffer = (float32_t *)malloc(core.iq_pairs-1 * sizeof(float32_t));

    int window_height = 800;
    int window_width = 800;
    float signal_window_portion = 0.6f;
    int signal_height = window_height * signal_window_portion;
    InitWindow(window_width, window_height, "dsp");
    SetAudioStreamBufferSizeDefault(core.iq_pairs / 5);
    InitAudioDevice();
    AudioStream audio_stream = LoadAudioStream(50000, 16, 1);
    PlayAudioStream(audio_stream);

    rlImGuiSetup(true);
    ImGuiIO *io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    RenderTexture2D scope = LoadRenderTexture(window_width, window_height);
    BeginTextureMode(scope);
    ClearBackground(BLACK);
    EndTextureMode();

    while (!WindowShouldClose())
    {
        core_error_t err = core_read_iq_pairs_sync(&core);
        if (err != CORE_ERROR_NONE) {
            printf("core error: %s", core_error_strings[err]);
            return err;
        }

        int demodulated_count = core.iq_pairs_buffer_count / 2 - 1;
        demodulate_fm_float32(core.iq_pairs_buffer, core.iq_pairs_buffer_count, demodulated_buffer, demodulated_count);

        size_t audio_count = 0;
        audio_count = decimate_block_average_float32(demodulated_buffer, demodulated_count, decimated_buffer, demodulated_count, decimate_factor);
        float32_rads_to_int16_audio(decimated_buffer, audio_count, audio_buffer, audio_count, core.audio_gain);

        if (IsAudioStreamProcessed(audio_stream))
        {
            UpdateAudioStream(audio_stream, audio_buffer, audio_count);
        }

        BeginTextureMode(scope);
            DrawRectangle(0, 0, window_width, window_height, (Color){0, 0, 0, 255});

            float lane = window_height / 3.0f;   // lane 0: IQ, lane 1: demod, lane 2: audio

            int n = window_width < demodulated_count ? window_width : demodulated_count;
            for (int x = 1; x < n; x++) {
                // raw I/Q, range ~[-128,127]
                DrawLine(x-1, lane_y(core.iq_pairs_buffer[(x-1)*2],   -140,140, 0, lane),
                        x,   lane_y(core.iq_pairs_buffer[x*2],       -140,140, 0, lane), (Color){80,160,255,255});
                DrawLine(x-1, lane_y(core.iq_pairs_buffer[(x-1)*2+1], -140,140, 0, lane),
                        x,   lane_y(core.iq_pairs_buffer[x*2+1],     -140,140, 0, lane), (Color){255,120,120,255});
                // demodulated phase, [-PI,PI]
                DrawLine(x-1, lane_y(demodulated_buffer[x-1], -PI,PI, lane, 2*lane),
                        x,   lane_y(demodulated_buffer[x],   -PI,PI, lane, 2*lane), RAYWHITE);
            }

            int an = window_width < (int)audio_count ? window_width : (int)audio_count;
            for (int x = 1; x < an; x++) {
                DrawLine(x-1, lane_y(audio_buffer[x-1], -32768,32767, 2*lane, 3*lane),
                        x,   lane_y(audio_buffer[x],   -32768,32767, 2*lane, 3*lane), (Color){120,255,160,255});
            }

            DrawLine(0, lane,   window_width, lane,   (Color){40,40,40,255});
            DrawLine(0, 2*lane, window_width, 2*lane, (Color){40,40,40,255});
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureRec(scope.texture,
                       (Rectangle){0, 0, (float)scope.texture.width, -(float)scope.texture.height},
                       (Vector2){0, 0}, WHITE);
        DrawFPS(10, 10);
        rlImGuiBegin();
        gui_draw(&core);
        rlImGuiEnd();
        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();

    free(audio_buffer);
    free(demodulated_buffer);
    rlImGuiShutdown();
    core_deinit(&core);
    return 0;
}