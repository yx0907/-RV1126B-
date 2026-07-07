#ifndef READ_H
#define READ_H

#include "lvgl/lvgl.h"


lv_obj_t *show_raw_from_file(const char *path, lv_obj_t *parent);

void image_close(void);

#endif