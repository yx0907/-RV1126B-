#ifndef OCR_EDIT_H
#define OCR_EDIT_H

#define OCR_EDIT_MAX 128

typedef enum
{
    OCR_EDIT_INSERT,
    OCR_EDIT_DELETE,
    OCR_EDIT_REPLACE
} ocr_edit_type_t;

typedef struct
{
    ocr_edit_type_t type;

    int item_index;

    int pos;

    char old_char;

    char new_char;

} ocr_edit_t;

typedef struct
{
    int count;

    ocr_edit_t edits[OCR_EDIT_MAX];

} ocr_edit_record_t;

void build_edit_record(
        int item_index,
        const char *old_text,
        const char *new_text,
        ocr_edit_record_t *record);

#endif


