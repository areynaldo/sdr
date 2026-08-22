#ifndef SDR_PLOTS_H
#define SDR_PLOTS_H

#include "base.h"

typedef enum plot_kind_t plot_kind_t;
enum plot_kind_t {
    PLOT_KIND_NONE,
    PLOT_KIND_SCATTER,
    PLOT_KIND_CURVE,
    PLOT_KIND_MAP,
    PLOT_KIND_COUNT,
};

typedef struct plot_t plot_t;
struct plot_t {
    color_t color;
};

#endif SDR_PLOTS_H