#ifndef UI_OCR_PAGE_H 
#define UI_OCR_PAGE_H 
#include "ocr/ocr_data.h" 
#include "lvgl/lvgl.h" 
#include "ocr/ocr_config.h"
static void create_char_conf( lv_obj_t *parent, ocr_item_t *item); 
static void create_card( lv_obj_t *parent, int index); 
//void save_btn_cb( lv_event_t *e); 
//void mode_btn_cb(lv_event_t *e); 
void savedo(void);
void ui_show_ocr_page(void * user_data); 
typedef struct { 
                  ocr_batch_t *batch; 
                  bool show_low_conf; 
                  bool auto_save; 
                  int mode; 
                  int frame_id; 
                  float threshold; 
                } ui_ocr_ctx_t; 
void ui_ocr_update(void *arg); 
extern lv_obj_t *ocr_area; 
extern lv_obj_t *keyboard; 
extern lv_obj_t *active_ta; 
void save_btn_cb(lv_event_t *e);
#endif
