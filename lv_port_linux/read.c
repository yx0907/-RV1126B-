#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "lvgl/lvgl.h"

#define IMG_W 1920
#define IMG_H 1080
#define IMG_SIZE (IMG_W * IMG_H)

/* =========================
 * 全局显示资源
 * ========================= */
static lv_obj_t *g_img_obj = NULL;
static lv_image_dsc_t g_img_dsc;
static long file_size = 0;

/* 当前映射buffer */
static uint8_t *g_mapped_buf = NULL;

/* 当前文件fd！！！ */
static int g_fd = -1;

/* =========================
 * 加载 RAW 文件并显示
 * ========================= */
lv_obj_t *show_raw_from_file(const char *path, lv_obj_t *parent)
{
    /* 1. 打开文件 */
    g_fd = open(path, O_RDONLY);
    if (g_fd < 0)
    {
        printf("[ERR] open failed\n");
        return NULL;
    }

    /* 2. 获取文件大小 */
    file_size = lseek(g_fd, 0, SEEK_END);
    lseek(g_fd, 0, SEEK_SET);

    printf("[INFO] file_size = %ld\n", file_size);

    if (file_size < IMG_SIZE)
    {
        printf("[ERR] file too small\n");
        close(g_fd);
        g_fd = -1;
        return NULL;
    }

    /* 3. mmap 映射 */
    g_mapped_buf = mmap(NULL,
                        file_size,
                        PROT_READ,
                        MAP_PRIVATE,
                        g_fd,
                        0);

    if (g_mapped_buf == MAP_FAILED)
    {
        printf("[ERR] mmap failed\n");
        close(g_fd);
        g_fd = -1;
        return NULL;
    }

    /* 4. 构建 LVGL 图像描述符 */
g_img_dsc.header.cf     = LV_COLOR_FORMAT_L8;
g_img_dsc.header.w      = IMG_W;
g_img_dsc.header.h      = IMG_H;
g_img_dsc.header.stride = IMG_W;
g_img_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;

g_img_dsc.data_size     = IMG_SIZE;      // ★★★★★
g_img_dsc.data          = g_mapped_buf;
    /* 5. 创建 LVGL 图像对象（只创建一次） */
    if (g_img_obj == NULL)
    {
        g_img_obj = lv_image_create(parent);
    }

    lv_image_set_src(g_img_obj, &g_img_dsc);

    /* 6. fd 可以关闭（mmap 已持有映射） */
    close(g_fd);
    g_fd = -1;

    return g_img_obj;
}

void image_close(void)
{
    if(g_img_obj)
    {
        /* 1. 让LVGL停止使用 */
        lv_image_set_src(g_img_obj, NULL);

        /* 2. 强制刷新UI */
        lv_obj_invalidate(g_img_obj);

        /* 3. 等一帧 */
        lv_timer_handler();
    }

    /* 4. 再释放buffer */
    if(g_mapped_buf)
    {
        munmap(g_mapped_buf, file_size);
        g_mapped_buf = NULL;
    }
}