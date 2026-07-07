#include "ocr_history.h"

#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char history_files[HISTORY_MAX_FILE][128];

int history_count=0;

static int cmp_session(const void *a, const void *b)
{
    int ida = 0;
    int idb = 0;

    sscanf((const char *)a, "Session%d", &ida);
    sscanf((const char *)b, "Session%d", &idb);

    return idb - ida;
}

void history_scan(void)
{
    history_count=0;

    DIR *dir=
        opendir("/home/elf/lvgl/save");

    if(dir==NULL)
        return;

    struct dirent *ent;

    while((ent=readdir(dir))!=NULL)
    {
        if(strncmp(ent->d_name,
                   "Session",
                   7)==0)
        {
            strcpy(history_files[history_count],
                   ent->d_name);

            history_count++;
        }
    }

    qsort(history_files,
      history_count,
      sizeof(history_files[0]),
      cmp_session);


    closedir(dir);
}