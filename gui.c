#include "gui.h"
#include "cimgui.h"
#include <math.h>
#define RC(r, g, b, a) {(r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a) / 255.0f}

static const ImVec4 RetroCol_Black = RC( 10,  10,  10, 255);
static const ImVec4 RetroCol_White = RC(255, 255, 255, 255);
static const ImVec4 RetroCol_Amber = RC(255, 176,   0, 255);
static const ImVec4 RetroCol_Clear = RC(  0,   0,   0,   0);
static const ImVec4 RetroCol_Scrim = RC(  0,   0,   0, 150);
 

static inline void gui_setup_style(void)
{
    ImGuiStyle* s = igGetStyle();
    ImVec4*     c = s->Colors;

    c[ImGuiCol_Text]                     = RetroCol_White;
    c[ImGuiCol_TextDisabled]             = RetroCol_White;
    c[ImGuiCol_WindowBg]                 = RetroCol_Black;
    c[ImGuiCol_ChildBg]                  = RetroCol_Clear;
    c[ImGuiCol_PopupBg]                  = RetroCol_Black;
    c[ImGuiCol_Border]                   = RetroCol_White;
    c[ImGuiCol_BorderShadow]             = RetroCol_Clear;
 
    c[ImGuiCol_FrameBg]                  = RetroCol_Black;
    c[ImGuiCol_FrameBgHovered]           = RetroCol_Amber;
    c[ImGuiCol_FrameBgActive]            = RetroCol_Black;
 
    c[ImGuiCol_TitleBg]                  = RetroCol_Black;
    c[ImGuiCol_TitleBgActive]            = RetroCol_Black;
    c[ImGuiCol_TitleBgCollapsed]         = RetroCol_Black;
    c[ImGuiCol_MenuBarBg]                = RetroCol_Black;
 
    c[ImGuiCol_ScrollbarBg]              = RetroCol_Black;
    c[ImGuiCol_ScrollbarGrab]            = RetroCol_White;
    c[ImGuiCol_ScrollbarGrabHovered]     = RetroCol_Amber;
    c[ImGuiCol_ScrollbarGrabActive]      = RetroCol_Amber;
 
    c[ImGuiCol_CheckMark]                = RetroCol_Black;
    c[ImGuiCol_CheckboxSelectedBg]       = RetroCol_Amber;
    c[ImGuiCol_InputTextCursor]          = RetroCol_Amber;
 
    c[ImGuiCol_SliderGrab]               = RetroCol_White;
    c[ImGuiCol_SliderGrabActive]         = RetroCol_Amber;
 
    c[ImGuiCol_Button]                   = RetroCol_Black;
    c[ImGuiCol_ButtonHovered]            = RetroCol_Amber;
    c[ImGuiCol_ButtonActive]             = RetroCol_Amber;
 
    c[ImGuiCol_Header]                   = RetroCol_Black;
    c[ImGuiCol_HeaderHovered]            = RetroCol_Amber;
    c[ImGuiCol_HeaderActive]             = RetroCol_Amber;
 
    c[ImGuiCol_Separator]                = RetroCol_White;
    c[ImGuiCol_SeparatorHovered]         = RetroCol_Amber;
    c[ImGuiCol_SeparatorActive]          = RetroCol_Amber;
 
    c[ImGuiCol_ResizeGrip]               = RetroCol_White;
    c[ImGuiCol_ResizeGripHovered]        = RetroCol_Amber;
    c[ImGuiCol_ResizeGripActive]         = RetroCol_Amber;
 
    c[ImGuiCol_Tab]                      = RetroCol_Black;
    c[ImGuiCol_TabHovered]               = RetroCol_Amber;
    c[ImGuiCol_TabSelected]              = RetroCol_Black;
    c[ImGuiCol_TabSelectedOverline]      = RetroCol_Amber;
    c[ImGuiCol_TabDimmed]                = RetroCol_Black;
    c[ImGuiCol_TabDimmedSelected]        = RetroCol_Black;
    c[ImGuiCol_TabDimmedSelectedOverline]= RetroCol_White;
 
    c[ImGuiCol_DockingPreview]           = RetroCol_Amber;
    c[ImGuiCol_DockingEmptyBg]           = RetroCol_Black;
 
    c[ImGuiCol_PlotLines]                = RetroCol_White;
    c[ImGuiCol_PlotLinesHovered]         = RetroCol_Amber;
    c[ImGuiCol_PlotHistogram]            = RetroCol_Amber;
    c[ImGuiCol_PlotHistogramHovered]     = RetroCol_White;
 
    c[ImGuiCol_TableHeaderBg]            = RetroCol_Black;
    c[ImGuiCol_TableBorderStrong]        = RetroCol_White;
    c[ImGuiCol_TableBorderLight]         = RetroCol_Clear;
    c[ImGuiCol_TableRowBg]               = RetroCol_Clear;
    c[ImGuiCol_TableRowBgAlt]            = RetroCol_Clear;
 
    c[ImGuiCol_TextLink]                 = RetroCol_Amber;
    c[ImGuiCol_TextSelectedBg]           = RetroCol_Amber;
    c[ImGuiCol_TreeLines]                = RetroCol_White;
 
    c[ImGuiCol_DragDropTarget]           = RetroCol_Amber;
    c[ImGuiCol_DragDropTargetBg]         = RetroCol_Amber;
    c[ImGuiCol_UnsavedMarker]            = RetroCol_Amber;
 
    c[ImGuiCol_NavCursor]                = RetroCol_Amber;
    c[ImGuiCol_NavWindowingHighlight]    = RetroCol_Amber;
    c[ImGuiCol_NavWindowingDimBg]        = RetroCol_Scrim;
    c[ImGuiCol_ModalWindowDimBg]         = RetroCol_Scrim;
 
    s->WindowRounding    = 0.0f;
    s->ChildRounding     = 0.0f;
    s->FrameRounding     = 0.0f;
    s->PopupRounding     = 0.0f;
    s->ScrollbarRounding = 0.0f;
    s->GrabRounding      = 0.0f;
    s->TabRounding       = 0.0f;
 
    s->WindowBorderSize  = 1.0f;
    s->ChildBorderSize   = 0.0f;
    s->PopupBorderSize   = 1.0f;
    s->FrameBorderSize   = 1.0f;
    s->TabBorderSize     = 0.0f;
 
    s->WindowPadding     = (ImVec2){ 8.0f, 8.0f };
    s->FramePadding      = (ImVec2){ 6.0f, 4.0f };
    s->ItemSpacing       = (ImVec2){ 8.0f, 6.0f };
    s->ItemInnerSpacing  = (ImVec2){ 6.0f, 4.0f };
    s->CellPadding       = (ImVec2){ 6.0f, 4.0f };
    s->ScrollbarSize     = 12.0f;
    s->GrabMinSize       = 10.0f;
    s->WindowTitleAlign  = (ImVec2){ 0.0f, 0.5f };
}

// TODO(areynaldo): do this
static inline void gui_setup_font(float size_px)
{
    ImGuiIO *io = igGetIO_Nil();
    ImFontConfig *cfg = ImFontConfig_ImFontConfig();

    cfg->OversampleH = 2;
    cfg->OversampleV = 2;
    cfg->PixelSnapH = true;

    /* Point this at whichever .ttf you ship. */
    ImFont *f = ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "fonts/IBMPLexMono-Regular.ttf", size_px, cfg, NULL);

    if (f == NULL)
        ImFontAtlas_AddFontDefault(io->Fonts, NULL);

    ImFontConfig_destroy(cfg);
}

#undef RC

gui_t gui_make(void)
{
    // gui_setup_style();
    // gui_setup_font();

    gui_t gui = (gui_t){0};
    gui.iq_plot = plot_make("IQ", -140.0f, 140.0f);
    gui.demod_plot = plot_make("Demod", -PI, PI);
    gui.audio_plot = plot_make("Audio", -32768.0f, 32767.0f);
    gui.audio_spectrum_plot = plot_make("Audio Spectrum", -100.0f, 0.0f);
    gui.audio_spectrogram = spectrogram_make("Audio Spectrogram", -100.0f, 0.0f, 512, 512);
    return gui;
}

void gui_destroy(gui_t *gui)
{
    plot_destroy(&gui->iq_plot);
    plot_destroy(&gui->demod_plot);
    plot_destroy(&gui->audio_plot);
    plot_destroy(&gui->audio_spectrum_plot);
    spectrogram_destroy(&gui->audio_spectrogram);
}

void gui_render_plots(gui_t *gui, core_t *core)
{
    size_t iq_count = core->iq_pairs_buffer_count / 2;
    plot_series_t iq_series[] = {
        {core->iq_pairs_buffer, iq_count, 2, PLOT_SAMPLE_FLOAT32, (color_t){80, 160, 255, 255}},
        {core->iq_pairs_buffer + 1, iq_count, 2, PLOT_SAMPLE_FLOAT32, (color_t){255, 120, 120, 255}},
    };
    plot_render(&gui->iq_plot, iq_series, 2);

    plot_series_t demod_series[] = {
        {core->demodulated_buffer, core->demodulated_buffer_count, 1, PLOT_SAMPLE_FLOAT32, (color_t){230, 230, 230, 255}},
    };
    plot_render(&gui->demod_plot, demod_series, 1);

    plot_series_t audio_series[] = {
        {core->audio_buffer, core->audio_buffer_count, 1, PLOT_SAMPLE_INT16, (color_t){120, 255, 160, 255}},
    };
    plot_render(&gui->audio_plot, audio_series, 1);

    plot_series_t audio_spectrum_series[] = {
        {core->audio_magnitude_buffer, core->audio_magnitude_buffer_count, 1,
         PLOT_SAMPLE_FLOAT32, (color_t){200, 160, 255, 255}},
    };
    plot_render(&gui->audio_spectrum_plot, audio_spectrum_series, 1);

    spectrogram_render(&gui->audio_spectrogram,
                       core->audio_magnitude_buffer,
                       core->audio_magnitude_buffer_count);
}

static void gui_plot_window(plot_t *plot)
{
    if (igBegin(plot->title, NULL, 0))
    {
        plot_to_gui(plot);
        if (igIsItemHovered(0))
        {
            ImGuiIO *io = igGetIO_Nil();
            plot_view_t *view = &plot->view;
            ImVec2_c item_min = igGetItemRectMin();
            float32_t local_x = io->MousePos.x - item_min.x;
            float32_t wheel = io->MouseWheel;
            if (wheel != 0.0f)
            {
                float32_t anchor = view->x_start + local_x * view->samples_per_pixel;
                view->samples_per_pixel *= powf(0.85f, wheel);
                if (view->samples_per_pixel < 0.001f)
                {
                    view->samples_per_pixel = 0.001f;
                }
                view->x_start = anchor - local_x * view->samples_per_pixel;
            }
            if (igIsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                ImVec2_c delta = igGetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
                view->x_start -= delta.x * view->samples_per_pixel;
                igResetMouseDragDelta(ImGuiMouseButton_Left);
            }
            if (igIsMouseDoubleClicked_Nil(ImGuiMouseButton_Left))
            {
                plot->initialized = false;
            }
        }
    }
    igEnd();
}

void gui_draw(gui_t *gui, core_t *core)
{
    igDockSpaceOverViewport(0, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
    if (igBegin("Settings", NULL, 0))
    {
        igSliderFloat("gain", &core->audio_gain, 0.0f, 16000.0f, "%.0f", 0);
        if (igSliderFloat("freq", &core->center_freq, SDR_FM_BAND_START, SDR_FM_BAND_END, "%.0f", 0))
        {
            core_set_center_freq(core, core->center_freq);
        }
        if (igInputFloat("##frequency_slider", &core->center_freq, 100000.0f, 1000000.0f, "%.0f", 0))
        {
            core->center_freq = CLAMP(core->center_freq, SDR_FM_BAND_START, SDR_FM_BAND_END);
            core_set_center_freq(core, core->center_freq);
        }
    }
    igEnd();

    gui_plot_window(&gui->iq_plot);
    gui_plot_window(&gui->demod_plot);
    gui_plot_window(&gui->audio_plot);
    gui_plot_window(&gui->audio_spectrum_plot);

    if (igBegin("Audio Spectrogram", NULL, 0))
    {
        spectrogram_to_gui(&gui->audio_spectrogram);
    }
    igEnd();
}