#include "ocr/ocr_config.h"

ocr_config_t g_ocr_cfg =
{
    .mode = OCR_MODE_SEMI_AUTO,
    .low_conf_threshold = 95.0f,
    .show_char_conf = true,
    .highlight_low_conf = true
};
