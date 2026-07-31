#include "gui.h"

#include "cimgui.h"
#include "rlImGui.h"

void gui_draw(core_t *core, RenderTexture2D *texture) {
    igDockSpaceOverViewport(0, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
    if (igBegin("Settings", NULL, 0)) {
        igSliderFloat("Gain", &core->audio_gain, 0.0f, 16000.0f, "%.0f", 0);
        if(igSliderFloat("##Center Frequency Slider", &core->center_freq, SDR_FM_BAND_START, SDR_FM_BAND_END, "%.0f", 0)) {
            core_set_center_freq(core, core->center_freq);
        }
        igSameLine(0, 0);
        if(igInputFloat("Center Frequency", &core->center_freq, SDR_FM_BAND_START, SDR_FM_BAND_END, "%.0f", 0)) {
            core_set_center_freq(core, core->center_freq);
        }

    }
    igEnd();

    if (igBegin("viewer", NULL, 0)) {
        rlImGuiImageRenderTexture(texture);
    }
    igEnd();
}