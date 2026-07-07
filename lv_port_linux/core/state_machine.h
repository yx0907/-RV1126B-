#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
//#include "ui/ui_state.h"

typedef enum
{
    STATE_MATTE = 0,     //等待

    STATE_HAKKEN,       //发现角铁

    STATE_CAPTURE,

    STATE_RTI,

    STATE_AI,

    STATE_SAVE,

    STATE_FINISH,
    
    STATE_NUM

    

}system_state_t;

typedef enum
{
    UI_STATE_MATTE,
    UI_STATE_HAKKEN,
    UI_STATE_CAPTURE,
    UI_STATE_RTI,
    UI_STATE_AI,
    UI_STATE_SAVE,
    UI_STATE_FINISH,
    

    UI_STATE_NUM   // 自动表示数量

} ui_state_index_t;

void state_machine_init(void);
void state_machine_set(system_state_t state, uint8_t is_error);


system_state_t state_machine_get(void);

typedef struct
{
    uint32_t start_time;

    uint32_t cost_time;

}state_time_t;

extern state_time_t state_time[];

void system_reset_batch(void);


#endif
