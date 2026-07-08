#ifndef _RTI_CONF_H
#define _RTI_CONF_H

/*********Generic configuration*********/

#define FRAME_WIDTH                             1920
#define FRAME_HEIGHT                            1080
#define CAMERA_PATHNAME                         "/dev/video23"
//#define CALIBRATE_LS_VECTOR                     

/*********V4L2 configuration*********/

#define RTI_V4L2_BUF_COUNT                      4
#define MAX_DROP                                4
#define MAX_PLANES                              2

/*********Light source driver configuration*********/

#define DRV_EN_PATHNAME                         "/dev/rti_ls_en"
#define DRV_ADDR_PATHNAME                       "/dev/rti_ls_addr"

/*********Process configuration*********/

#define LVGL_EXEC_PATHNAME                      "/home/elf/lvgl/main"
#define FINDER_COMMAND                          "/home/elf/elf-env/bin/python3 /home/elf/paddle/paddle1.py"
#define OCR_COMMAND                             "/home/elf/elf-env/bin/python3 /home/elf/paddle/paddle2.py"
#define PASS_IO_PATHNAME                        "/sys/class/gpio/gpio169/value" 
#define ALARM_IO_PATHNAME                       "/sys/class/gpio/gpio170/value" 
#define IO_INIT_PATHNAME                        "/sys/class/gpio/export"           
#define PASS_IO_DIR_PATHNAME                    "/sys/class/gpio/gpio169/direction"
#define ALARM_IO_DIR_PATHNAME                   "/sys/class/gpio/gpio170/direction"


#endif
