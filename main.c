#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "raylib.h"
#include "rtl-sdr.h"

int8_t uint8_to_centered_int8(uint8_t value)
{
    return (int8_t)((int)value - 128);
}

// TODO(areynaldo): maye it should just take the whole buffer and a length instead of single
float demodulate_fm(float i_prev, float q_prev, float i_next, float q_next)
{
    float real = i_prev * i_next + q_prev * q_next;
    float imaginary = i_prev * q_next - q_prev * i_next;
    float demodulated = atan2f(imaginary, real);

    return demodulated;
}

int main(void)
{
    uint32_t sample_rate = 250000;
    uint32_t center_freq = 97300000;
    size_t iq_pairs = 4096;
    int8_t *iq_buffer = NULL;
    size_t iq_buffer_size = iq_pairs * 2;

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
    iq_buffer = (int8_t *)malloc(iq_buffer_size * sizeof(int8_t));
    float *demodulated_buffer = (float *)malloc(iq_pairs * sizeof(float));
    int16_t *audio_buffer = (int16_t *)malloc((iq_pairs / 5 + 1) * sizeof(int16_t));

    int window_height = 800;
    int window_width = 800;
    float signal_window_portion = 0.6f;
    int signal_height = window_height * signal_window_portion;
    InitWindow(window_width, window_height, "dsp");
    SetAudioStreamBufferSizeDefault(iq_pairs / 5);
    InitAudioDevice();
    AudioStream audio_stream = LoadAudioStream(50000, 16, 1);
    PlayAudioStream(audio_stream);

    while (!WindowShouldClose())
    {
        int read_count = 0;
        if (rtlsdr_read_sync(dev, iq_buffer, iq_buffer_size, &read_count) < 0 || read_count == 0)
        {
            // TODO: defer clean stuff
            printf("read_sync failed / returned 0 bytes.\n");
            free(iq_buffer);
            rtlsdr_close(dev);
            return 1;
        }
        int pairs_read = read_count/2;

        // TODO(areynaldo): consider unrolling
        // center the buffer
        for (int i = 0; i < read_count; i++)
        {
            iq_buffer[i] = iq_buffer[i] ^ 0x80;
        }

        int demodulated_count = pairs_read - 1;
        for (int i = 0; i < demodulated_count; i++)
        {
            demodulated_buffer[i] = demodulate_fm(
                (float)iq_buffer[i * 2],
                (float)iq_buffer[i * 2 + 1],
                (float)iq_buffer[(i + 1) * 2],
                (float)iq_buffer[(i + 1) * 2 + 1]);
        }

        // TODO(areynaldo): moving-average as crude low pass, FIR later
        // decimate 250kHz -> 50kHz (factor of 5) and convert to int16_t
        int audio_count = 0;
        for (int i = 0; i + 5 <= demodulated_count; i += 5)
        {
            float accumulator = 0.0f;
            for (int k = 0; k < 5; k++)
            {
                accumulator += demodulated_buffer[i + k];
            }

            float sample = (accumulator / 5.0f) * 8000.0f;

            if (sample > 32767.0f)
            {
                sample = 32767.0f;
            }

            if (sample < -32768.0f)
            {
                sample = -32768.0f;
            }

            audio_buffer[audio_count++] = (int16_t)sample;
        }

        if(IsAudioStreamProcessed(audio_stream)) {
            UpdateAudioStream(audio_stream, audio_buffer, audio_count);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10, 10);
        for (int x = 0; x < window_width; x++)
        {
            int8_t i_sample = iq_buffer[x * 2];
            int8_t q_sample = iq_buffer[x * 2 + 1];
            DrawPixel(x, window_height / 2 + (i_sample * ((float)signal_height / 256.0f)), BLUE);
            DrawPixel(x, window_height / 2 + (q_sample * ((float)signal_height / 256.0f)), RED);
            DrawPixel(x, window_height / 2 + (demodulated_buffer[x] * ((float)signal_height * 0.25f / PI)), WHITE);
        }
        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();

    free(audio_buffer);
    free(demodulated_buffer);
    free(iq_buffer);
    rtlsdr_close(dev);
    return 0;
}