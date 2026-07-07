#ifndef OCR_HISTORY_H
#define OCR_HISTORY_H

#define HISTORY_MAX_FILE 100

extern char history_files[HISTORY_MAX_FILE][128];
extern int history_count;

void history_scan(void);

#endif