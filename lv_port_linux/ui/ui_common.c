#include "ui_common.h"
#include "ui/ui_state.h"
#include "read.h"
#include "lvgl/lvgl.h"
#include "ui/ui_read.h"
#include <stdio.h>
#include "ocr/ocr_data.h"
#include "ui/ui_ocr_page.h"
#include "receive/ai_comm.h"
#include "ui/ui_history.h"

/* 外部引用 */
extern lv_obj_t *g_img_obj;
static lv_obj_t *naka;
lv_obj_t *label2;
/*
以下定义为代替一些基本固定可需要整体性更改的数据

*/
#define Z_TOP_BAR_H      40     //顶部栏高度
#define Z_BOTTOM_BAR_H   39     //底部栏高度
#define Z_ALL_LONG 1024         //屏幕长度
#define Z_NAKA_H 530            //中间部分高度（高度600顶部栏高度与底部栏高度）

static lv_obj_t *ocr_container = NULL;
static lv_obj_t *ocr_label_list[32];   // 最多32行（可改）
static uint8_t ocr_line_num = 0;
    

lv_style_t style_text;
lv_style_t style_panel;
uint8_t i = 0;



static lv_obj_t *cpu_label;
static lv_obj_t *npu_label;

uint8_t abcd =0;


//uint8_t su =0;
void img_open(void)
{
    lv_obj_t *child = lv_obj_get_child(naka, 0);
        lv_obj_t *na = show_raw_from_file("/home/elf/HCimage.gray",child); 
        lv_obj_set_scrollbar_mode(na, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_img_recolor_opa(na, LV_OPA_0, 0);
        lv_obj_set_style_opa(na, LV_OPA_COVER, 0);
}

void img_close(void)
{
image_close();
}
//！！！
static void btn_event_cb3(lv_event_t *e)
{
     state_machine_set(abcd,0);
     abcd++;
     if(abcd>=7)abcd=0;
}
//！！！1

static void btn_event_cb4(lv_event_t *e)
{
    // state_machine_set(abcd-1,1);
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CANCEL || code == LV_EVENT_READY)
    {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, NULL);
        active_ta = NULL;
    }
}

static void rekisi_btn_cb(lv_event_t *e)
{
    ui_history_show();
}


static void perf_timer_cb(lv_timer_t *t)
{
    char buf[64];

    float cpu = get_cpu_usage();
    float npu = get_npu_usage();

    snprintf(buf,
             sizeof(buf),
             "CPU: %.1f%%",
             cpu);

    lv_label_set_text(cpu_label,
                      buf);

    snprintf(buf,
             sizeof(buf),
             "NPU: %.1f%%",
             npu);

    lv_label_set_text(npu_label,
                      buf);
}



void ui_create_top_bar(lv_obj_t *parent)
{
//设置顶部栏及标签

    lv_obj_t *top;

    top = lv_obj_create(parent);


    lv_obj_set_size(top, Z_ALL_LONG, Z_TOP_BAR_H);

    lv_obj_align(top,LV_ALIGN_TOP_MID,0,0);

    lv_obj_set_style_bg_color(top,lv_color_hex(0xF2F4F7),0);

    lv_obj_set_style_radius(top, 0, 0); //设置圆角

    lv_obj_set_style_border_width(top, 0, 0);   //设置边框宽度
    
    lv_obj_set_scrollbar_mode(top, LV_SCROLLBAR_MODE_OFF);//这两个关闭滑动条与弹性滑动
    lv_obj_set_scroll_dir(top, LV_DIR_NONE);
    
        lv_obj_t *title;

    title = lv_label_create(top);
    lv_label_set_text(title,"AI赋能设计，设计点亮AI  光度立体AI智能识别");
    lv_obj_set_style_text_font(title, &noto_sans_font, LV_PART_MAIN);

    lv_obj_align(title,LV_ALIGN_LEFT_MID,10,0);



//设置“统计”按钮

    lv_obj_t *btn2;

    btn2 = lv_btn_create(top);

    lv_obj_set_size(btn2, 70, 28);

    lv_obj_align(btn2,LV_ALIGN_TOP_MID,-15,-15);

    lv_obj_t *label2;

    label2 = lv_label_create(btn2);

    lv_label_set_text(label2, "统计");
    //lv_obj_add_event_cb(btn2, btn_event_cb2, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_radius(btn2, 0, 0); //设置圆角

    lv_obj_set_style_border_width(btn2, 0, 0);   //设置边框宽度
    lv_obj_set_style_text_font(label2,&noto_sans_font,LV_PART_MAIN);

    lv_obj_center(label2);

//设置“检查”按钮
    lv_obj_t *btn3;

    btn3 = lv_btn_create(top);

    lv_obj_set_size(btn3, 70, 28);

    lv_obj_align(btn3,LV_ALIGN_TOP_RIGHT,0,-15);

    lv_obj_t *label3;

    label3 = lv_label_create(btn3);

    //lv_obj_add_event_cb(btn3, btn_event_cb1, LV_EVENT_CLICKED, NULL);

    lv_label_set_text(label3, "检查");
        lv_obj_set_style_radius(btn3, 0, 0); //设置圆角

    lv_obj_set_style_border_width(btn3, 0, 0);   //设置边框宽度
    lv_obj_set_style_text_font(label3,&noto_sans_font,LV_PART_MAIN);

    lv_obj_center(label3);

    //两个测试按钮
   
    lv_obj_t *btn7;

    btn7 = lv_btn_create(top);

    lv_obj_set_size(btn7, 70, 28);

    lv_obj_align(btn7,LV_ALIGN_TOP_MID,285,-15);

    lv_obj_t *label7;

    label7 = lv_label_create(btn7);

    lv_obj_add_event_cb(btn7, btn_event_cb3, LV_EVENT_CLICKED, NULL);

    lv_label_set_text(label7, "加上");
        lv_obj_set_style_radius(btn7, 0, 0); //设置圆角

    lv_obj_set_style_border_width(btn7, 0, 0);   //设置边框宽度
    lv_obj_set_style_text_font(label7,&noto_sans_font,LV_PART_MAIN);

    lv_obj_center(label7);


    lv_obj_t *btn8;

    btn8 = lv_btn_create(top);

    lv_obj_set_size(btn8, 70, 28);

    lv_obj_align(btn8,LV_ALIGN_TOP_MID,165,-15);

    lv_obj_t *label8;

    label8 = lv_label_create(btn8);

    lv_obj_add_event_cb(btn8, btn_event_cb4, LV_EVENT_CLICKED, NULL);

    lv_label_set_text(label8, "减去");
        lv_obj_set_style_radius(btn8, 0, 0); //设置圆角

    lv_obj_set_style_border_width(btn8, 0, 0);   //设置边框宽度
    lv_obj_set_style_text_font(label8,&noto_sans_font,LV_PART_MAIN);

    lv_obj_center(label8);
    
}

void ui_create_naka_bar(lv_obj_t *parent)
{

    
        naka = lv_obj_create(parent);
        lv_obj_set_size(naka, Z_ALL_LONG, Z_NAKA_H);
        lv_obj_set_style_pad_all(naka, 0, 0);
        lv_obj_align(naka,LV_ALIGN_TOP_MID,0,40);
        lv_obj_set_style_bg_color(naka,lv_color_black(),0);
        lv_obj_set_style_radius(naka, 0, 0); //设置圆角
        lv_obj_set_scrollbar_mode(naka, LV_SCROLLBAR_MODE_OFF);//这两个关闭滑动条与弹性滑动
        lv_obj_set_scroll_dir(naka, LV_DIR_NONE);
        //lv_fs_stdio_init();
        lv_obj_t *img_area = lv_obj_create(naka);
lv_obj_set_size(img_area, 844, 256);
lv_obj_align(img_area, LV_ALIGN_TOP_LEFT, 180, 0);

lv_obj_set_style_bg_color(img_area, lv_color_black(), 0);
lv_obj_set_style_border_width(img_area, 0, 0);

//ocr_area 
lv_obj_t *ocr_mae = lv_obj_create(naka);
ui_ready = 1;
printf("[UI]READY\n");
lv_obj_set_size(ocr_mae, 844, 265);
lv_obj_align(ocr_mae, LV_ALIGN_TOP_LEFT, 180, 280);

lv_obj_set_style_bg_color(ocr_mae, lv_color_hex(0x111111), 0);
        lv_obj_set_style_radius(ocr_mae, 0, 0); //设置圆角
        lv_obj_set_scrollbar_mode(ocr_mae, LV_SCROLLBAR_MODE_OFF);//这两个关闭滑动条与弹性滑动
        lv_obj_set_scroll_dir(ocr_mae, LV_DIR_NONE);
        lv_obj_set_style_pad_all(ocr_mae,0,LV_PART_MAIN);
ocr_area = lv_obj_create(ocr_mae);
lv_obj_set_size(ocr_area, 844, 210);
lv_obj_align(ocr_area,LV_ALIGN_TOP_LEFT,0,0);
lv_obj_set_style_bg_color(ocr_area, lv_color_hex(0x111111), 0);
lv_obj_set_style_border_width(ocr_area, 0, 0);
lv_obj_set_style_pad_all(ocr_mae,0,LV_PART_MAIN);

lv_obj_set_flex_flow(ocr_area, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_row(ocr_area, 6, 0);
lv_obj_set_style_pad_left(ocr_area, 10, 0);
lv_obj_set_style_pad_top(ocr_area, 10, 0);
//此乃放置图片的一横栏
        lv_obj_t *natairo = lv_obj_create(img_area);
        //lv_obj_t *image;
        lv_obj_set_size(natairo,844,265);
        //lv_obj_set_pos(natairo,400,40);
        lv_obj_set_style_pad_all(natairo, 0, 0);
        lv_obj_set_style_radius(natairo, 0, 0); //设置圆角
        lv_obj_set_style_pad_all(natairo, -10, 0);
        lv_obj_align(natairo,LV_ALIGN_TOP_LEFT,180,0); 
        lv_obj_set_scroll_dir(natairo, LV_DIR_ALL);
        lv_obj_set_scrollbar_mode(natairo, LV_SCROLLBAR_MODE_AUTO);

lv_obj_t *footer = lv_obj_create(ocr_mae);
lv_obj_set_size(footer, 844, 55);
lv_obj_align(footer,LV_ALIGN_TOP_LEFT,0,200);
        lv_obj_set_style_radius(footer, 0, 0); //设置圆角
        lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);//这两个关闭滑动条与弹性滑动
        lv_obj_set_scroll_dir(footer, LV_DIR_NONE);
lv_obj_set_style_bg_color(footer, lv_color_hex(0x222222), 0);
lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

lv_obj_t *btn_save = lv_btn_create(footer);
lv_obj_align(btn_save, LV_ALIGN_LEFT_MID, 20, 0);
lv_obj_set_size(btn_save,100,40);
lv_obj_set_style_radius(btn_save, 0, 0);
lv_obj_t *label1 = lv_label_create(btn_save);
lv_label_set_text(label1, "保存");
lv_obj_center(label1);
lv_obj_set_style_text_font(label1, &noto_sans_font, LV_PART_MAIN);



lv_obj_t *btn_rekisi =lv_btn_create(footer); 
lv_obj_set_size(btn_rekisi,100,40);    
lv_obj_set_style_radius(btn_rekisi, 0, 0);
lv_obj_align(btn_rekisi,LV_ALIGN_RIGHT_MID, -20, 0);
lv_obj_t *lab =lv_label_create(btn_rekisi);
lv_label_set_text(lab,"历史");
lv_obj_center(lab);
lv_obj_add_event_cb(btn_rekisi,rekisi_btn_cb,LV_EVENT_CLICKED,NULL);

lv_obj_set_style_text_font(lab, &noto_sans_font, LV_PART_MAIN);

//lv_obj_add_event_cb(btn_mode, mode_btn_cb, LV_EVENT_CLICKED, NULL);

lv_obj_add_event_cb(btn_save, save_btn_cb, LV_EVENT_CLICKED, NULL);        


    keyboard = lv_keyboard_create(ocr_mae);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(keyboard, kb_event_cb, LV_EVENT_ALL, NULL);


    //实时系统状态显示
        ui_state_create(naka);

//ocr active
lv_obj_clean(ocr_area);

lv_obj_t *label = lv_label_create(ocr_area);



}

void ui_create_bottom_bar(lv_obj_t *parent)
{

    lv_obj_t *bottom;

     bottom = lv_obj_create(parent);

     lv_obj_set_size(bottom,Z_ALL_LONG,Z_BOTTOM_BAR_H);

     lv_obj_align(bottom,LV_ALIGN_BOTTOM_MID,0,10);

     lv_obj_set_style_bg_color(bottom,lv_color_white(),0);
    lv_obj_set_style_radius(bottom,0,0);

    lv_obj_set_style_border_width(bottom,0,0);
    
         lv_obj_set_scrollbar_mode(bottom, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(bottom, LV_DIR_NONE);

    cpu_label = lv_label_create(bottom);
    npu_label = lv_label_create(bottom);
    
    lv_obj_set_pos(npu_label,150,0);

    float cpu = get_cpu_usage();
    char buf[64];

snprintf(buf,
         sizeof(buf),
         "CPU: %.1f%%",
         cpu);


    float npu = get_npu_usage();
    lv_obj_align(cpu_label,LV_ALIGN_LEFT_MID,0,-5);
lv_label_set_text(cpu_label,
                  buf);
snprintf(buf,
         sizeof(buf),
         "NPU: %.1f%%",
         npu);
lv_obj_align(npu_label,LV_ALIGN_LEFT_MID,150,-5);
lv_label_set_text(npu_label,
                  buf);



lv_timer_create(perf_timer_cb, 500, NULL);


}


void ui_style_init(void)
{
    lv_style_init(&style_text);

    lv_style_set_text_color(&style_text, lv_color_white());
    lv_style_set_text_font(&style_text, &lv_font_montserrat_20);
}

