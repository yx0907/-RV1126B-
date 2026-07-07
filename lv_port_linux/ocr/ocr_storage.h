#ifndef OCR_STORAGE_H 
#define OCR_STORAGE_H 
#include "ocr/ocr_data.h" 
#include "ocr/ocr_edit.h" 
void save_batch( const ocr_batch_t *batch); 
void save_edit_record( const ocr_edit_record_t *record); 
int ocr_get_next_task_id(void);
void ocr_update_task_id(int id);
void append_result(ocr_batch_t *batch);
void ocr_storage_init(void);
void ocr_storage_reset(void);
#endif
