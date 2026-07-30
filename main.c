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
#include "raylib.h"
#include "cimgui.h"
#include "rlImGui.h"

static inline float lane_y(float v, float vmin, float vmax, float top, float bot) {
    float t = (v - vmin) / (vmax - vmin);
    return bot - t * (bot - top);
}

int main(void)
{
    float gain = 8000.0f;
    uint32_t sample_rate = 250000;
    uint32_t center_freq = 97300000;
    size_t iq_pairs = 4096;
    uint8_t *iq_uint8_buffer = NULL;
    size_t iq_uint8_buffer_size = iq_pairs * 2;

    // detect devices
    uint32_t device_count = rtlsdr_get_device_count();
    printf("Devices found: %u\n", device_count);
    if (device_count == 0)
    {
        printf("No RTL-SDR detected. On Windows, connect the device and install the WinUSB driver on the dongle with Zadig, then rerun.\n");
        return 1;
    }

    // list devices
    for (uint32_t i = 0; i < device_count; i++)
    {
        printf("  [%u] %s\n", i, rtlsdr_get_device_name(i));
    }

    // open device
    rtlsdr_dev_t *dev = NULL;
    if (rtlsdr_open(&dev, 0) < 0)
    {
        printf("Failed to open device 0 (in use by another app, or driver issue).\n");
        return 1;
    }

    // read info string
    char manu[256] = {0};
    char prod[256] = {0};
    char serial[256] = {0};
    rtlsdr_get_usb_strings(dev, manu, prod, serial);
    printf("Opened: %s %s (serial %s)\n", manu, prod, serial);

    // set automatic gain
    rtlsdr_set_tuner_gain_mode(dev, 0);

    // set sample rate
    rtlsdr_set_sample_rate(dev, sample_rate);

    // set freq
    rtlsdr_set_center_freq(dev, center_freq);

    printf("Sample rate: %u Hz, Center: %u Hz\n",
           rtlsdr_get_sample_rate(dev), rtlsdr_get_center_freq(dev));

    rtlsdr_reset_buffer(dev);
    iq_uint8_buffer = (uint8_t *)malloc(iq_uint8_buffer_size * sizeof(uint8_t));
    float *iq_buffer = (float *)malloc(iq_uint8_buffer_size * sizeof(float32_t));
    float *demodulated_buffer = (float *)malloc(iq_pairs * sizeof(float));
    uint32_t decimate_factor = 5;
    size_t audio_buffer_count = (iq_pairs / decimate_factor + 1) * sizeof(int16_t);
    int16_t *audio_buffer = (int16_t *)malloc(audio_buffer_count);
    float32_t *decimated_buffer = (float32_t *)malloc(iq_pairs-1 * sizeof(float32_t));

    int window_height = 800;
    int window_width = 800;
    float signal_window_portion = 0.6f;
    int signal_height = window_height * signal_window_portion;
    InitWindow(window_width, window_height, "dsp");
    SetAudioStreamBufferSizeDefault(iq_pairs / 5);
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
        int iq_read_count = 0;
        if (rtlsdr_read_sync(dev, iq_uint8_buffer, iq_uint8_buffer_size, &iq_read_count) < 0 || iq_read_count == 0)
        {
            // TODO: defer clean stuff
            printf("read_sync failed / returned 0 bytes.\n");
            free(iq_uint8_buffer);
            free(iq_buffer);
            rtlsdr_close(dev);
            return 1;
        }
        uint8_buffer_to_float32_buffer(iq_uint8_buffer, iq_read_count, iq_buffer, iq_read_count);

        int demodulated_count = iq_read_count / 2 - 1;
        demodulate_fm_float32(iq_buffer, iq_read_count, demodulated_buffer, demodulated_count);

        size_t audio_count = 0;
        audio_count = decimate_block_average_float32(demodulated_buffer, demodulated_count, decimated_buffer, demodulated_count, decimate_factor);
        float32_rads_to_int16_audio(decimated_buffer, audio_count, audio_buffer, audio_count, gain);

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
                DrawLine(x-1, lane_y(iq_buffer[(x-1)*2],   -140,140, 0, lane),
                        x,   lane_y(iq_buffer[x*2],       -140,140, 0, lane), (Color){80,160,255,255});
                DrawLine(x-1, lane_y(iq_buffer[(x-1)*2+1], -140,140, 0, lane),
                        x,   lane_y(iq_buffer[x*2+1],     -140,140, 0, lane), (Color){255,120,120,255});
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
        igDockSpaceOverViewport(0, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
        if (igBegin("Controls", NULL, 0)) {
            igSliderFloat("Gain", &gain, 0.0f, 16000.0f, "%.0f", 0);
        }
        igEnd();
        rlImGuiEnd();
        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();

    free(audio_buffer);
    free(demodulated_buffer);
    free(iq_buffer);
    rlImGuiShutdown();
    rtlsdr_close(dev);
    return 0;
}