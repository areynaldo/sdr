#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "base.h"
#include "dsp.h"
#include "dsp.c"
#include "rtl-sdr.h"
#include "fft.h"
#include "fft.c"
#include "core.h"
#include "core.c"
#include "visualization.h"
#include "visualization_raylib.c"
#include "spectrogram.h"
#include "spectrogram.c"
#include "gui.h"
#include "gui.c"

#include "raylib.h"
#include "rlImGui.h"

int main(void)
{
    core_t core = core_init((core_t){0});
    setup_fm_pipeline(&core, 5, 50000, sizeof(int16_t), 1);

    int window_width = 1280;
    int window_height = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "dsp");

    gui_t gui = gui_make();

    SetAudioStreamBufferSizeDefault(core.iq_pairs_count / core.decimate_factor);
    InitAudioDevice();
    AudioStream audio_stream = LoadAudioStream(core.audio_sample_rate, 16, core.audio_channels);
    PlayAudioStream(audio_stream);
    UpdateAudioStream(audio_stream, core.audio_buffer, core.audio_buffer_count);

    rlImGuiSetup(true);
    ImGuiIO *io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    gui_setup_style();

    while (!WindowShouldClose())
    {
        core_error_t err = core_read_iq_pairs_sync(&core);
        if (err != CORE_ERROR_NONE)
        {
            printf("core error: %s", core_error_strings[err]);
            break;
        }

        run_fm_pipeline(&core);

        if (IsAudioStreamProcessed(audio_stream))
        {
            printf("hi?\n");
            UpdateAudioStream(audio_stream, core.audio_buffer, core.audio_buffer_count);
        }

        gui_render_plots(&gui, &core);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10, 10);
        rlImGuiBegin();

        gui_draw(&gui, &core);
        rlImGuiEnd();
        EndDrawing();
    }

    gui_destroy(&gui);
    rlImGuiShutdown();
    CloseAudioDevice();
    CloseWindow();
    core_deinit(&core);
    return 0;
}