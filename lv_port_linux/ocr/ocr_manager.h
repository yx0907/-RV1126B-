#ifndef OCR_MANAGER_H
#define OCR_MANAGER_H

#include "ocr/ocr_data.h"

void ocr_process_batch(
        ocr_batch_t *batch);
void ocr_push_result(ocr_batch_t *batch);

extern int busy;
void ocr_process_batch(ocr_batch_t *batch);

#endif
