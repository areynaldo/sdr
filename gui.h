#ifndef SDR_GUI_H
#define SDR_GUI_H

#include "core.h"
#include "visualization.h"
#include "spectrogram.h"

typedef struct gui_t
{
    plot_t iq_plot;
    plot_t demod_plot;
    plot_t audio_plot;
    plot_t audio_spectrum_plot;
    spectrogram_t audio_spectrogram;
} gui_t;

gui_t gui_make(void);
void gui_destroy(gui_t *gui);
void gui_render_plots(gui_t *gui, core_t *core);
void gui_draw(gui_t *gui, core_t *core);

#endif