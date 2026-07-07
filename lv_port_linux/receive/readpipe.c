#include <stdio.h>      
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <fcntl.h>      
#include <errno.h>      
#include "lvgl/lvgl.h"  
#include "core/state_machine.h"

static int pipe_fd;

static void pipe_timer_cb(lv_timer_t *timer);
int buf = 0;


static void pipe_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    int value;
    int n;
    int last_value = -1;

    while(1)
    {
        n = read(pipe_fd, &value, sizeof(value));

        if(n == sizeof(value))
        {
            last_value = value;
        }
        else
        {
            break;
        }
    }

    if(last_value != -1)
    {
        state_machine_set(last_value, 0);
    }
}

static int pipe_fd = -1;

int readpipe_init(int fd)
{
    pipe_fd = fd;
    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);
    lv_timer_t *t = lv_timer_create(pipe_timer_cb, 20, NULL);
    return 0;
}