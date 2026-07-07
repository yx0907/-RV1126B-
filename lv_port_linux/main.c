#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl/src/misc/lv_fs.h"
#include "read.h"
#include "ui/ui_read.h"
#include "ui/ui_manager.h"
#include "receive/ai_comm.h"
#include "ocr/ocr_storage.h"
#include "lvgl/src/widgets/calendar/lv_calendar_header_arrow.h"
#include "lvgl/src/widgets/calendar/lv_calendar_header_dropdown.h"
#include "lvgl/src/widgets/calendar/lv_calendar.h"
#include "lvgl/src/widgets/calendar/lv_calendar_chinese.h"
#include "core/state_machine.h"
#include "receive/readpipe.h"
#define DISP_BUF_SIZE (600 * 1024)



static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ? : dflt;
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    // const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    // lv_display_t * disp = lv_linux_drm_create();

    // lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_SDL_VIDEO_WIDTH") ? : "800");
    const int height = atoi(getenv("LV_SDL_VIDEO_HEIGHT") ? : "480");

    lv_sdl_window_create(width, height);
}
#else
#error Unsupported configuration
#endif


int main(int argc, char *argv[])
{
    /* Initialize LVGL */
    lv_init();
    lv_fs_posix_init();
    /* DRM device node */
    const char *device = "/dev/dri/card0";
    /* Create a DRM display */
    lv_display_t *disp = lv_linux_drm_create();
    /* Set DRM device file and connector */
    /* The 2nd argument is the DRM device path */
    /* The 3rd argument is the connector_id (-1 = auto-select first available) */
    lv_linux_drm_set_file(disp, device, -1);
    /* Create demo widgets */
    //lv_demo_widgets();
  	  
    ui_init();//初始化ui，使ui界面显示
            
    

   state_machine_init();//状态机初始化
    ai_comm_init();//接受ai方面的初始化
    ocr_storage_init();//处理接受的文件相关初始化
    /* Handle LVGL tasks */
    lv_indev_t * touch =lv_evdev_create(
                                            LV_INDEV_TYPE_POINTER,
                                            "/dev/input/event2"
                                  );//创建输入设备，即触摸屏
    int fd=0;
if(argc >= 2)
{
     fd = atoi(argv[1]);
    readpipe_init(fd);
}
else
{
    printf("Standalone mode.\n");
}

  
    lv_indev_set_display(touch, disp);

while(1)
{
lv_tick_inc(5);
    lv_timer_handler();

    usleep(5000);
}
    return 0;
}
