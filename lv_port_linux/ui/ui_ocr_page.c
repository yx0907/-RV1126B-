#include "ui/ui_ocr_page.h" 
#include "ocr/ocr_storage.h"
// #include "ocr/ocr_edit.h" 
#include "lvgl/lvgl.h" 
#include <string.h> 
#include <stdio.h> 
#include "ocr/ocr_manager.h" 
#include <stdlib.h> 
#include "ui/ui_common.h" 
#include "receive/fifot.h"
#define OCR_AREA_W 844 
#define OCR_AREA_H 265 
#define CARD_W 820 
#define CARD_H 250

static ocr_batch_t *g_batch; 
static bool g_editable;
static int ui_ready = 0;

#define OCR_CONF_HIGH      0.95f
#define OCR_CONF_MID       0.60f


lv_obj_t *keyboard = NULL; 
lv_obj_t *active_ta = NULL;

static ocr_mode_t ui_mode = OCR_MODE_SEMI_AUTO;

static ocr_batch_t *ui_batch = NULL; 
int busy = 0; 
static lv_obj_t *g_textarea[OCR_ITEM_MAX]; 
lv_obj_t *ocr_area = NULL;

static void textarea_event_cb(lv_event_t *e) 
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);
  
  if(code == LV_EVENT_FOCUSED) 
  {
  
      active_ta = ta; 
      /* 绑定键盘 */ 
      lv_keyboard_set_textarea(keyboard, ta); 
      /* 显示键盘 */ 
      lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN); 
      /* 置顶，避免被遮挡 */ lv_obj_move_foreground(keyboard); 
      
  }
  
  /* 失去焦点隐藏键盘 */ 
  if(code == LV_EVENT_DEFOCUSED) 
    { 
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); 
      lv_keyboard_set_textarea(keyboard, NULL); 
      active_ta = NULL; 
    } 
}

static void create_char_conf(lv_obj_t *parent, ocr_item_t *item)
{
    lv_obj_t *span_group = lv_spangroup_create(parent);

    lv_obj_set_width(span_group, 500);

    lv_spangroup_set_mode(span_group, LV_SPAN_MODE_BREAK);

    lv_obj_set_style_bg_opa(span_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(span_group, 0, 0);

    for(int i = 0; i < item->char_num; i++)
    {
        lv_span_t *span = lv_spangroup_new_span(span_group);

        char buf[32];

        snprintf(buf,
                 sizeof(buf),
                 "%c(%.0f%%) ",
                 item->chars[i].c,
                 item->chars[i].conf * 100);

        lv_span_set_text(span, buf);

        lv_style_t *style = lv_span_get_style(span);

        lv_style_set_text_font(style, &noto_sans_font);

        if(item->chars[i].conf >= OCR_CONF_HIGH)
        {
            lv_style_set_text_color(style,
                                    lv_color_white());
        }
        else if(item->chars[i].conf >= OCR_CONF_MID)
        {
            lv_style_set_text_color(style,
                                    lv_palette_main(LV_PALETTE_YELLOW));
        }
        else
        {
            lv_style_set_text_color(style,
                                    lv_palette_main(LV_PALETTE_RED));
        }
    }
}


static void create_card(lv_obj_t *parent,int index) 
    { 
        ocr_item_t *items =&g_batch->items[index]; 
        lv_obj_t *card =lv_obj_create(parent); 
        lv_obj_set_size(card,CARD_W,CARD_H); 
        lv_obj_set_pos(card,10,index*(CARD_H+5)); 
        create_char_conf(card,items); 
        lv_obj_t *conf =lv_label_create(card); 
        lv_label_set_text_fmt(conf,"%.1f%%",items->total_conf);
        
        lv_obj_align(conf,LV_ALIGN_TOP_RIGHT,-10,5); 
        g_textarea[index] =lv_textarea_create(card); 
        lv_obj_set_size(g_textarea[index],300,35); 
        lv_obj_align(g_textarea[index],LV_ALIGN_RIGHT_MID,-10,10); 
        lv_textarea_set_one_line(g_textarea[index],true); 
        lv_textarea_set_text(g_textarea[index],items->text);
        
        if(!g_editable) 
          { 
              lv_obj_add_state(g_textarea[index],LV_STATE_DISABLED); 
          } 
          
      }
      


void savedo(void) 
{ 
    printf("savedo() begin\n");

if(!ui_batch)
{
    printf("ui_batch == NULL\n");
    return;
}

printf("item_num=%d\n", ui_batch->item_num); 
     
    for(int i = 0; i < ui_batch->item_num; i++) 
        { 
            const char *new_text = lv_textarea_get_text(g_textarea[i]); 
            
            strncpy(ui_batch->items[i].text,new_text,OCR_TEXT_MAX-1);
            ui_batch->items[i].text[OCR_TEXT_MAX - 1] = '\0'; 
        } 
        
         
        append_result(ui_batch); 
        
}

void save_btn_cb(lv_event_t *e)
{

    printf("savedo() begin\n");

if(!ui_batch)
{
    printf("ui_batch == NULL\n");
    return;
}

printf("item_num=%d\n", ui_batch->item_num); 
     
    for(int i = 0; i < ui_batch->item_num; i++) 
        { 
            const char *new_text = lv_textarea_get_text(g_textarea[i]); 
            
            strncpy(ui_batch->items[i].text,new_text,OCR_TEXT_MAX-1);
            ui_batch->items[i].text[OCR_TEXT_MAX - 1] = '\0'; 
        } 
        
         
        append_result(ui_batch); 
        fifot();
}

void ui_show_ocr_page(void *arg) 
{ 
    ui_ocr_update(arg); 
} 

void ui_ocr_update(void *arg) 
{ 
    if(ocr_area == NULL) 
        { 
            printf("[UI] ocr_area not ready\n"); return;
        } 
    ocr_batch_t *batch = (ocr_batch_t *)arg; 
    if(!batch) return; 
    lv_obj_clean(ocr_area); 
    lv_obj_set_flex_flow(ocr_area, LV_FLEX_FLOW_COLUMN); 
    lv_obj_set_style_pad_row(ocr_area, 6, 0); 
    lv_obj_set_style_pad_all(ocr_area, 8, 0); 
    //!!!调试用，开始
    printf("item_num=%d\n", batch->item_num);
printf("OCR_ITEM_MAX=%d\n", OCR_ITEM_MAX);

if(batch->item_num > OCR_ITEM_MAX)
{
    printf("ERROR: item_num overflow!\n");
    return;
}

//!!!调试用，结束
    for(int i = 0; i < batch->item_num; i++) 
        { 
            lv_obj_t *row = lv_obj_create(ocr_area); 
            lv_obj_set_size(row, 820, 200); 
            lv_obj_set_style_bg_color(row, lv_color_hex(0x222222), 0); 
            lv_obj_set_style_pad_all(row, 5, 0); 
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN); 
            lv_obj_set_style_pad_row(row, 5, 0);
            lv_obj_set_style_pad_column(row, 10, 0); 
            /* 左侧文本 */ 
            create_char_conf(row,
                 &batch->items[i]);
            /* 右侧输入框 */ 
            g_textarea[i] = lv_textarea_create(row); 
            lv_textarea_set_one_line(g_textarea[i], true); 
            lv_textarea_set_text(g_textarea[i], batch->items[i].text); 
            lv_obj_set_width(g_textarea[i], 300); 
            lv_obj_set_style_text_color(g_textarea[i], lv_color_black(), LV_PART_MAIN); 
            lv_obj_set_style_text_color( g_textarea[i], lv_color_black(), LV_PART_CURSOR); 
            lv_obj_set_style_text_color( g_textarea[i], lv_color_black(), LV_PART_SELECTED); 
            lv_obj_set_style_text_color(g_textarea[i], lv_color_black(), 0); 
            /* 关键：键盘绑定 */ 
            lv_obj_add_event_cb( g_textarea[i], textarea_event_cb, LV_EVENT_ALL, (void*)i ); 
            
          } 
          
          ui_batch = batch; 
}
