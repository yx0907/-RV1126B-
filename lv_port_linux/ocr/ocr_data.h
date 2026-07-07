#ifndef OCR_DATA_H
#define OCR_DATA_H

#define OCR_ITEM_MAX      30
#define OCR_TEXT_MAX      64

#include <stdbool.h>
#include <stdint.h>


#define NET_MAGIC 0xAABBCCDD

#define MAX_OCR_ITEM 10

#define MAX_CHAR_PER_ITEM 64



#pragma pack(push, 1)
typedef struct
{
    char c;            // 字符
    float conf;        // 置信度
} ocr_char_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    char text[64];
    float total_conf;
    int char_num;
    ocr_char_t chars[32];
} ocr_item_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct
{
    int item_num;
    ocr_item_t items[MAX_OCR_ITEM];

} ocr_batch_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct
{
    uint32_t magic;        // 协议标识
    uint32_t frame_id;     // 帧编号（防乱序）
    uint32_t length;       // 数据长度（可选，用于TCP拆包）

    uint8_t mode;          // AI模式（可选扩展）
    uint8_t reserved[3];   // 对齐/扩展

    ocr_batch_t batch;     // OCR数据主体

} net_packet_t;
#pragma pack(pop)



#endif
