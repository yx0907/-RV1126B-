#ifndef UI_READ_H
#define UI_READ_H

#include "lvgl/lvgl.h"

typedef struct {
    unsigned long user, nice, system, idle;
} cpu_stat_t;

float get_npu_usage(void);
float get_cpu_usage();
void read_cpu(cpu_stat_t *s);

#endif
