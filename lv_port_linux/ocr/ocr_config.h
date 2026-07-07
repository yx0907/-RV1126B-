#ifndef OCR_CONFIG_H
#define OCR_CONFIG_H

#include <stdbool.h>

typedef enum
{
    OCR_MODE_AUTO,          // 全自动
    OCR_MODE_SEMI_AUTO,     // 半自动
    OCR_MODE_MANUAL         // 手动
} ocr_mode_t;

typedef struct
{
    ocr_mode_t mode;

    float low_conf_threshold;

    bool show_char_conf;

    bool highlight_low_conf;

} ocr_config_t;

extern ocr_config_t g_ocr_cfg;

#endif
