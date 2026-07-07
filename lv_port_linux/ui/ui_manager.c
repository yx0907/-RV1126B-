#include "ui_manager.h"
#include "ui_main.h"

#include "lvgl/lvgl.h"

static lv_obj_t *scr_main;

void ui_init(void)
{
    scr_main = ui_main_create();

    lv_scr_load(scr_main);
}
