#include "ocr/ocr_manager.h"
//#include "ocr/ocr_config.h"
#include "ocr/ocr_storage.h"
#include "ui/ui_ocr_page.h"
#include "lvgl/lvgl.h"
#include <string.h>
#include <stdio.h>


static bool batch_need_confirm(
        ocr_batch_t *batch)
{
    int i;

    if(g_ocr_cfg.mode ==
            OCR_MODE_MANUAL)
    {
        return true;
    }

    if(g_ocr_cfg.mode ==
            OCR_MODE_AUTO)
    {
        return false;
    }

    for(i=0;
        i<batch->item_num;
        i++)
    {
        if(batch->items[i].total_conf
                < g_ocr_cfg.low_conf_threshold)
        {
            return true;
        }
    }
    if(batch->item_num > OCR_ITEM_MAX)
    return NULL;
    return false;
}

void ocr_process_batch(ocr_batch_t *batch)
{

    ocr_batch_t *copy = lv_malloc(sizeof(ocr_batch_t));
    if(!copy) return;

    memcpy(copy, batch, sizeof(ocr_batch_t));

   
    for(int i = 0; i < copy->item_num; i++)
    {
        strncpy(copy->items[i].text,
                batch->items[i].text,
                OCR_TEXT_MAX - 1);

        copy->items[i].text[OCR_TEXT_MAX - 1] = '\0';
    }

    lv_async_call(ui_ocr_update, copy);

    
    printf("[OCR] process done\n");
}
void ocr_push_result(ocr_batch_t *batch)
{
   
    ocr_batch_t *copy = lv_malloc(sizeof(ocr_batch_t));

    memcpy(copy, batch, sizeof(ocr_batch_t));

    lv_async_call(ui_show_ocr_page, copy);
}



