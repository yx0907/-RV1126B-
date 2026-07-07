#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "ui/ui_read.h"


static cpu_stat_t prev, curr;

void read_cpu(cpu_stat_t *s)
{
    FILE *f = fopen("/proc/stat", "r");
    if(!f) return;

    fscanf(f, "cpu %lu %lu %lu %lu",
           &s->user,
           &s->nice,
           &s->system,
           &s->idle);

    fclose(f);
}

float get_cpu_usage()
{
    cpu_stat_t s;

    read_cpu(&s);

    unsigned long prev_idle = prev.idle;
    unsigned long idle = s.idle;

    unsigned long prev_total =
        prev.user + prev.nice + prev.system + prev.idle;

    unsigned long total =
        s.user + s.nice + s.system + s.idle;

    float totald = total - prev_total;
    float idled  = idle - prev_idle;

    prev = s;

    if(totald == 0)
        return 0;

    return (1.0f - idled / totald) * 100.0f;
}

float get_npu_usage(void)
{
    FILE *fp;
    int load = 0;

    fp = fopen("/proc/rknpu/load", "r");
    if(fp == NULL)
    {
        return -1.0f;
    }

    fscanf(fp, "NPU load: %d%%", &load);

    fclose(fp);

    return (float)load;
}

