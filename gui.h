#ifndef SDR_GUI_H
#define SDR_GUI_H

#include "base.h"

typedef sdr_gui_internal_t;

typedef struct sdr_gui_t
{
    sdr_gui_internal_t *internal;
} sdr_gui_t;

void sdr_gui_init(sdr_gui_t *gui);
void sdr_gui_deinit(sdr_gui_t *gui);

void sdr_gui_start_frame(sdr_gui_t *gu);
void sdr_gui_end_frame(sdr_gui_t *gui);

#endif
