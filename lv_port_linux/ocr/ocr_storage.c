#include "ocr_storage.h" 
#include <stdio.h> 

static FILE *g_session_fp = NULL;

static int g_session_id = 0;

static int load_session_index(void);
static void save_session_index(int id);
void save_batch(const ocr_batch_t *batch) 
{ FILE *fp = fopen("/home/elf/lvgl/save/ocr.txt", "a+"); 
if (!fp) return; 
for (int i = 0; i < batch->item_num; i++) 
{ 
fprintf(fp, "%s\n", batch->items[i].text); 
} 
fclose(fp); 
}

int ocr_get_next_task_id(void)
{
    FILE *fp;
    int id = 1;

    fp = fopen("/home/elf/lvgl/save/task_index.txt","r");

    if(fp)
    {
        fscanf(fp,"%d",&id);
        fclose(fp);
    }

    return id;
}

void ocr_update_task_id(int id)
{
    FILE *fp;

    fp = fopen("/home/elf/lvgl/save/task_index.txt","w");

    if(fp == NULL)
        return;

    fprintf(fp,"%d",id);

    fclose(fp);
}

void append_result(ocr_batch_t *batch)
{
    printf("g_session_fp=%p\n", g_session_fp);
    static int save_id=1;

    fprintf(g_session_fp,
            "------------- Save %03d -------------\n",
            save_id++);

    fprintf(g_session_fp,
            "Item Num : %d\n\n",
            batch->item_num);

    for(int i=0;i<batch->item_num;i++)
    {
        fprintf(g_session_fp,
                "%02d : %s\n",
                i+1,
                batch->items[i].text);
    }

    fprintf(g_session_fp,"\n");

    fflush(g_session_fp);
}

void ocr_storage_init(void)
{
    printf("storage 1\n");
    fflush(stdout);

    g_session_id = load_session_index();

    printf("storage 2\n");
    fflush(stdout);

    char name[128];

    sprintf(name,
            "/home/elf/lvgl/save/Session%06d.txt",
            g_session_id);

    printf("file=%s\n", name);
    fflush(stdout);

    g_session_fp = fopen(name, "w");

    printf("fp=%p\n", (void *)g_session_fp);
    fflush(stdout);

    if(g_session_fp == NULL)
    {
        perror("fopen");
        return;
    }

    fprintf(g_session_fp,
            "=========================\n");

    fprintf(g_session_fp,
            "OCR Session %06d\n",
            g_session_id);

    fprintf(g_session_fp,
            "=========================\n\n");

    fflush(g_session_fp);

    save_session_index(g_session_id + 1);

    printf("storage ok\n");
    fflush(stdout);
}

static int load_session_index(void)
{
    FILE *fp = fopen("session_index.txt", "r");

    int id = 1;

    if(fp)
    {
        fscanf(fp, "%d", &id);
        fclose(fp);
    }

    return id;
}

static void save_session_index(int id)
{
    FILE *fp = fopen("session_index.txt", "w");

    if(fp)
    {
        fprintf(fp,"%d",id);
        fclose(fp);
    }
}

void ocr_storage_reset(void)
{
    if(g_session_fp)
    {
        fclose(g_session_fp);
        g_session_fp = NULL;
    }

    g_session_id = 1;

    save_session_index(2);

    char name[128];

    sprintf(name,
        "/home/elf/lvgl/save/Session%06d.txt",
        g_session_id);

    g_session_fp = fopen(name,"w");

    fprintf(g_session_fp,
            "=========================\n");
    fprintf(g_session_fp,
            "OCR Session %06d\n",
            g_session_id);
    fprintf(g_session_fp,
            "=========================\n\n");

    fflush(g_session_fp);
}
