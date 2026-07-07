#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl.h"

void ui_create_top_bar(lv_obj_t *parent);
void ui_create_bottom_bar(lv_obj_t *parent);
void ui_create_naka_bar(lv_obj_t *parent);
extern lv_obj_t *label2;
void img_open(void);
void img_close(void);


#endif
