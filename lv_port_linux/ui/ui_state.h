#ifndef UI_STATE_H
#define UI_STATE_H

#include "lvgl/lvgl.h"
#include "core/state_machine.h"



typedef struct
{
    lv_obj_t *row;   // 每一行容器
    lv_obj_t *dot;   // 状态圆点
    lv_obj_t *name;  // 状态名称
    lv_obj_t *time;  // 耗时标签
} ui_state_item_t;

void ui_state_create(lv_obj_t *parent);

void ui_state_update(system_state_t state,
                     uint8_t error_flag,
                     system_state_t error_state);

void ui_state_reset(void);
void ui_state_set_hakken_state(int state);
void ui_total_time_update(uint32_t ms);

#endif
