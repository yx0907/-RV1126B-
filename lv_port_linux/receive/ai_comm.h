#ifndef AI_COMM_H
#define AI_COMM_H

#include "ocr/ocr_data.h"

#include "ocr/ocr_manager.h"
#include "ocr/ocr_data.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "external/cjson/cJSON.h"
#include "receive/ai_parser.h"
#include "lvgl/lvgl.h"
#include <stdlib.h>

void ai_comm_init(void);
void convert_packet_to_batch(
        net_packet_t *in,
        ocr_batch_t *out);

void ai_json_to_batch(char *buf, ocr_batch_t *out);

void ai_comm_init(void);
extern int ui_ready;
extern int Internetlink;

#endif
