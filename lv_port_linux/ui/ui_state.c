#include "ui_state.h"
#include <stdio.h>
#include <string.h>
#include "core/state_machine.h"

static ui_state_item_t g_state_items[UI_STATE_NUM];
static int g_hakken_state = 0;

uint8_t su2 =2;
static lv_obj_t *g_total_time_label;





static const char *state_name[] =
{
    "等待",
    "发现角铁",
    "拍照",
    "光度立体处理",
    "AI识别",
    "保存结果",
    "完成"
};

static lv_color_t get_state_color(system_state_t state, system_state_t current)
{
    if(state == current)
        return lv_palette_main(LV_PALETTE_YELLOW);

    if(state < current)
        return lv_palette_main(LV_PALETTE_GREEN);

    return lv_palette_main(LV_PALETTE_GREY);
}

void ui_state_create(lv_obj_t *parent)
{
    // 左侧状态栏容器
    lv_obj_t *cont = lv_obj_create(parent);

    lv_obj_set_size(cont, 180, 530);

    lv_obj_set_style_bg_color(cont, lv_color_hex(0x202020), 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 0, 0);
    lv_obj_set_style_radius(cont, 0, 0);

    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

    // 初始化每一行
    for(int i = 0; i < UI_STATE_NUM; i++)
    {
        // 每一行容器
        g_state_items[i].row = lv_obj_create(cont);

        lv_obj_set_size(g_state_items[i].row, 180, 40);

        lv_obj_set_style_bg_color(
            g_state_items[i].row,
            lv_color_hex(0x2A2A2A),
            0);

        lv_obj_set_style_pad_all(g_state_items[i].row, 5, 0);
        lv_obj_set_style_radius(g_state_items[i].row, 0, 0);
        lv_obj_remove_flag(
            g_state_items[i].row,
            LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_layout(
            g_state_items[i].row,
            LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(
            g_state_items[i].row,
            LV_FLEX_FLOW_ROW);

        lv_obj_set_flex_align(
            g_state_items[i].row,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        // ===== 圆点 =====
        g_state_items[i].dot = lv_obj_create(g_state_items[i].row);

        lv_obj_set_size(g_state_items[i].dot, 10, 10);

        lv_obj_set_style_radius(
            g_state_items[i].dot,
            LV_RADIUS_CIRCLE,
            0);

        lv_obj_set_style_bg_color(
            g_state_items[i].dot,
            lv_palette_main(LV_PALETTE_GREY),
            0);

        lv_obj_set_style_bg_opa(
            g_state_items[i].dot,
            LV_OPA_COVER,
            0);

        lv_obj_set_style_border_width(g_state_items[i].dot, 0, 0);

        lv_obj_set_flex_grow(g_state_items[i].dot, 0);

        // ===== 状态名称 =====
        g_state_items[i].name = lv_label_create(g_state_items[i].row);

        if(i == STATE_HAKKEN)
{
    char buf[64];

    switch(g_hakken_state)
    {
        case 1:
            snprintf(buf,
                     sizeof(buf),
                     "发现角铁：未识别到");
            break;

        case 2:
            snprintf(buf,
                     sizeof(buf),
                     "发现角铁：继续");
            break;

        case 3:
            snprintf(buf,
                     sizeof(buf),
                     "发现角铁：正在居中");
            break;

        case 4:
            snprintf(buf,
                     sizeof(buf),
                     "发现角铁：居中");
            break;

        default:
            snprintf(buf,
                     sizeof(buf),
                     "发现角铁");
            break;
    }

    lv_label_set_text(g_state_items[i].name, buf);
}
else
{
    lv_label_set_text(
        g_state_items[i].name,
        state_name[i]);
}

        lv_obj_set_style_text_color(
            g_state_items[i].name,
            lv_color_hex(0xDDDDDD),
            0);

        lv_obj_set_style_text_font(
            g_state_items[i].name,
            &noto_sans_font,
            0);

        // ===== 耗时 =====
        g_state_items[i].time = lv_label_create(g_state_items[i].row);

        lv_label_set_text(g_state_items[i].time, "");

        lv_obj_set_style_text_color(
            g_state_items[i].time,
            lv_color_hex(0xAAAAAA),
            0);

        lv_obj_set_style_text_font(
            g_state_items[i].time,
            &lv_font_montserrat_12,
            0);
    }
    g_total_time_label = lv_label_create(cont);

lv_label_set_text(
    g_total_time_label,
    "总用时:0ms");

lv_obj_set_style_text_font(
    g_total_time_label,
    &noto_sans_font,
    0);

lv_obj_set_style_text_color(
    g_total_time_label,
    lv_color_hex(0x00FF88),
    0);
    lv_obj_set_width(
    g_total_time_label,
    170);

lv_obj_set_style_text_align(
    g_total_time_label,
    LV_TEXT_ALIGN_RIGHT,
    0);
}

void ui_state_update(system_state_t state,
                     uint8_t error_flag,
                     system_state_t error_state)
{
    for(int i = 0; i < UI_STATE_NUM; i++)
    {
        lv_color_t c;

        // =========================
        // 1. 错误优先级最高
        // =========================
        if(error_flag && i == error_state)
        {
            c = lv_palette_main(LV_PALETTE_RED);
        }
        else if(i == state)
        {
            c = lv_palette_main(LV_PALETTE_YELLOW);
        }
        else if(i < state)
        {
            c = lv_palette_main(LV_PALETTE_GREEN);
        }
        else
        {
            c = lv_palette_main(LV_PALETTE_GREY);
        }

        lv_obj_set_style_bg_color(g_state_items[i].dot, c, 0);

        // =========================
        // 2. 名称显示
        // =========================
        if(i == STATE_HAKKEN)
{
    char buf[64];

    switch(g_hakken_state)
    {
        case 1:
            strcpy(buf, "发现角铁：未识别到");
            break;

        case 2:
            strcpy(buf, "发现角铁：继续");
            break;

        case 3:
            strcpy(buf, "发现角铁：正在居中");
            break;

        case 4:
            strcpy(buf, "发现角铁：居中");
            break;

        default:
            strcpy(buf, "发现角铁");
            break;
    }

    lv_label_set_text(g_state_items[i].name, buf);
}
else
{
    lv_label_set_text(
        g_state_items[i].name,
        state_name[i]);
}

        // =========================
        // 3. 时间显示
        // =========================

        if(i == STATE_MATTE)
        {
            lv_label_set_text(g_state_items[i].time, "");
            continue;
        }



        // 已完成状态显示历史时间
        else if(i < state)
        {
            char buf[32];
            snprintf(buf, sizeof(buf),
                     "%dms",
                     state_time[i].cost_time);

            lv_label_set_text(g_state_items[i].time, buf);
        }

        // 错误状态不显示时间
        if(error_flag && i == error_state)
        {
            lv_label_set_text(g_state_items[i].time, "ERROR");
        }
    }
}
void ui_state_reset(void)
{
    g_hakken_state = 0;
    for(int i = 0; i < UI_STATE_NUM; i++)
    {
        // 全部变灰
        lv_obj_set_style_bg_color(
            g_state_items[i].dot,
            lv_palette_main(LV_PALETTE_GREY),
            0);

        // 清空时间
        lv_label_set_text(
            g_state_items[i].time,
            "");

        // 恢复默认名称
        lv_label_set_text(
            g_state_items[i].name,
            state_name[i]);
    }
    lv_obj_set_style_bg_color(
    g_state_items[STATE_MATTE].dot,
    lv_palette_main(LV_PALETTE_YELLOW),
    0);
}


void ui_total_time_update(uint32_t ms)
{
    char buf[64];

    float sec = ms / 1000.0f;

    snprintf(buf,
             sizeof(buf),
             "总用时：%.3fs",
             sec);

    lv_label_set_text(
        g_total_time_label,
        buf);
}

void ui_state_set_hakken_state(int state)
{
    g_hakken_state = state;

    /* 如果当前就在发现阶段，立即刷新 */
    if(state_machine_get() == STATE_HAKKEN)
    {
        ui_state_update(
            STATE_HAKKEN,
            0,
            STATE_MATTE);
    }
}
