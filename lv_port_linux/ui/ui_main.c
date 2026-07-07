#include "ui_main.h"
#include "lvgl/lvgl.h"
#include "ui/ui_common.h"
#include "ui/ui_state.h"
#include "ui/ui_read.h"

static void ui_create_middle_area(lv_obj_t *parent);
extern void ui_ocr_update(void *arg);

lv_obj_t *ui_main_create(void)
{
    lv_obj_t *scr;

    scr = lv_obj_create(NULL);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);//这两个关闭滑动条与弹性滑动
    lv_obj_set_scroll_dir(scr, LV_DIR_NONE);
    lv_obj_set_style_bg_color(
            scr,
            lv_color_hex(0x1E1E1E),
            0);

    ui_create_top_bar(scr);

    ui_create_bottom_bar(scr);



    ui_create_middle_area(scr);
 ui_create_naka_bar(scr);
 state_machine_set(0,0);

 

    return scr;
}


static void ui_create_middle_area(lv_obj_t *parent)
{

}
