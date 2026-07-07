#include "state_machine.h"
#include "ui/ui_state.h"
#include "lvgl/lvgl.h"
#include "ui/ui_ocr_page.h"
#include "ui/ui_common.h"
#include <stdio.h>
#include <string.h>

static uint32_t state_enter_tick;
static system_state_t current_state;
static uint8_t error_flag = 0;
static system_state_t error_state = STATE_MATTE;
static uint32_t active_start_tick;
static uint8_t active_started = 0;

state_time_t state_time[STATE_NUM];
static uint32_t total_start_tick;
static uint32_t zong_tick;

void state_machine_init(void)
{
    current_state = STATE_MATTE;
    state_enter_tick = lv_tick_get();
    total_start_tick = state_enter_tick;
        zong_tick = lv_tick_get() - total_start_tick;
}

void system_reset_batch(void)
{
    memset(state_time, 0, sizeof(state_time));

    current_state = STATE_MATTE;
    state_enter_tick = lv_tick_get();
    total_start_tick = state_enter_tick;


    active_started = 0;
    active_start_tick = 0;

    ui_state_reset();
    ui_total_time_update(0);
}

void state_machine_set(system_state_t state, uint8_t is_error)
{
    //修改
    if(state==STATE_SAVE)
    {
        printf("enter STATE_SAVE\n");
        savedo();
        state=STATE_FINISH;
        
    }
    if(state==STATE_AI)
    {
        img_close();
        img_open();
    }
    uint32_t now = lv_tick_get();
        if(state == STATE_MATTE)
    {
        system_reset_batch();
        return;
    }

    // =========================
    // 1. 记录时间
    // =========================
    state_time[current_state].cost_time =
        now - state_enter_tick;

    // =========================
    // 2. 处理错误
    // =========================
    if(is_error)
    {
        error_flag = 1;
        error_state = state;   // 记录错误发生在哪一步
    }
    if(is_error==0)
    {
        error_flag = 0;
        error_state = STATE_MATTE; 

    }

    // =========================
    // 3. 正常状态切换
    // =========================
    current_state = state;
    state_enter_tick = now;

    // =========================
    // 4. UI更新
    // =========================
    ui_state_update(state, error_flag, error_state);

    // ui_total_time_update(
    // now - total_start_tick);
//     if(state == STATE_FINISH)
// {
//     ui_total_time_update(
//         now - total_start_tick);

           // =========================
    // 4. 有效计时逻辑（核心）
    // =========================

    // 从“第一个有效步骤”开始计时
    if(state != STATE_MATTE && state != STATE_FINISH)
    {
        if(active_started == 0)
        {
            active_started = 1;
            active_start_tick = now;
        }
    }

    // =========================
    // 5. 结束时计算总时间
    // =========================
    if(state == STATE_FINISH)
    {
        if(active_started)
        {
            ui_total_time_update(
                now - active_start_tick);
        }
}
}

system_state_t state_machine_get(void)
{
    return current_state;
}
