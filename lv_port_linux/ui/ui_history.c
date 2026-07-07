#include "ui_history.h"
#include "ocr/ocr_history.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include "ui/ui_main.h"
#include "ocr/ocr_storage.h"
#include <string.h>
#include <dirent.h>
#include <unistd.h>


static lv_obj_t *history_list = NULL;
static lv_obj_t *history_ta = NULL;

char file_buffer[32768];

static void history_click_cb(lv_event_t *e)
{

    int idx = (intptr_t)lv_event_get_user_data(e);

    history_read_file(history_files[idx]);


    lv_textarea_set_text(history_ta, file_buffer);
}

void history_read_file(const char *file)
{
    char path[256];

    sprintf(path,
            "/home/elf/lvgl/save/%s",
            file);

    FILE *fp=fopen(path,"r");

    if(fp==NULL)
        return;

    file_buffer[0]='\0';

    char line[256];

    while(fgets(line,sizeof(line),fp))
    {
        strcat(file_buffer,line);
    }

    fclose(fp);
}

static void reset_session_index(void)
{
        // session_index.txt 清零
    FILE *fp = fopen("/home/elf/lvgl/session_index.txt", "w");
    if(fp)
    {
        fprintf(fp, "0\n");
        fclose(fp);
    }
}
static void delete_history_files(void)
{
    const char *dir_path = "/home/elf/lvgl/save/";
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        // 跳过 . ..
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char path[256];
        snprintf(path, sizeof(path),
                 "%s%s", dir_path, entry->d_name);

        unlink(path);   // 删除文件
    }

    closedir(dir);
}
static void history_reset_cb(lv_event_t *e)
{
 
    delete_history_files();
    reset_session_index();
    ocr_storage_reset();
    printf("[HISTORY] reset done\n");
}

static void history_hana_cb(lv_event_t *e)
{
        lv_obj_add_flag(history_list,LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_flag(history_ta,LV_OBJ_FLAG_HIDDEN);


}


void ui_history_show(void)
{
    /* 获取当前页面 */
    lv_obj_t *dangqian = lv_screen_active();

    /* 清空页面 */
    

    /* 扫描Session文件 */
    history_scan();

    /*
    左侧 Session 列表
     */

    history_list = lv_list_create(dangqian);

    lv_obj_set_size(history_list,
                    250,
                    567);

    lv_obj_align(history_list,
                 LV_ALIGN_TOP_MID,
                 -387,
                 0);
lv_obj_set_style_radius(history_list, 0, 0);
    /*
     右侧 TextArea
     */

    history_ta = lv_textarea_create(dangqian);

    lv_obj_set_size(history_ta,
                    776,
                    567);

    lv_obj_align(history_ta,
                 LV_ALIGN_TOP_MID,
                 120,
                 0);
lv_obj_set_style_radius(history_ta, 0, 0);
lv_obj_t *label = lv_obj_get_child(history_ta, 0);

printf("label=%p\n", label);
    lv_textarea_set_text(history_ta,file_buffer);

    /* 设置为只读 */
lv_textarea_set_password_mode(history_ta, false);

    /* 白色文字 */
    lv_obj_set_style_text_color(history_ta,
                                lv_color_black(),
                                0);

    /*
     添加所有 Session 文件
     */
    lv_obj_t *btn_back = lv_btn_create(history_list);
lv_obj_set_style_radius(btn_back, 0, 0);
lv_obj_t *label_back = lv_label_create(btn_back);
lv_label_set_text(label_back, "返回");
lv_obj_set_style_text_font(label_back, &noto_sans_font, LV_PART_MAIN);
lv_obj_add_event_cb(btn_back,
                    history_hana_cb,
                    LV_EVENT_CLICKED,
                    NULL);


    for(int i = 0; i < history_count; i++)
    {
        lv_obj_t *btn;

        btn = lv_list_add_btn(
                    history_list,
                    LV_SYMBOL_FILE,
                    history_files[i]);

        lv_obj_add_event_cb(
                    btn,
                    history_click_cb,
                    LV_EVENT_CLICKED,
                    (void*)(intptr_t)i);
    }
    lv_obj_t *btn_reset = lv_btn_create(history_list);
lv_obj_set_style_radius(btn_reset, 0, 0);
lv_obj_set_pos(btn_reset,150,0);
lv_obj_t *label_reset = lv_label_create(btn_reset);
lv_label_set_text(label_reset, "重置");
lv_obj_set_style_text_font(label_reset, &noto_sans_font, LV_PART_MAIN);
lv_obj_add_event_cb(btn_reset,
                    history_reset_cb,
                    LV_EVENT_CLICKED,
                    NULL);

    /* 
    没有历史文件
     */

    if(history_count == 0)
    {
        lv_textarea_set_text(history_ta,
                             "No OCR History.");
    }

}
