#include "ai_parser.h"
#include "external/cjson/cJSON.h"
#include <string.h>
#include <stdlib.h>

ocr_batch_t *parse_json_to_batch(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if(!root) return NULL;

    ocr_batch_t *batch = malloc(sizeof(ocr_batch_t));
    memset(batch, 0, sizeof(ocr_batch_t));

    cJSON *items = cJSON_GetObjectItem(root, "items");
    if(!items)
    {
        cJSON_Delete(root);
        return batch;
    }

    int count = cJSON_GetArraySize(items);
    batch->item_num = count;

    if(count > MAX_OCR_ITEM)
        count = MAX_OCR_ITEM;

    for(int i = 0; i < count; i++)
    {
        cJSON *it = cJSON_GetArrayItem(items, i);

        cJSON *text = cJSON_GetObjectItem(it, "text");

        if(text && text->valuestring)
        {
            strncpy(batch->items[i].text,
                    text->valuestring,
                    sizeof(batch->items[i].text) - 1);
        }

        cJSON *conf = cJSON_GetObjectItem(it, "total_conf");
        if(conf) batch->items[i].total_conf = conf->valuedouble;

        // chars 同理：逐个 memcpy
        cJSON *chars = cJSON_GetObjectItem(it, "chars");
        if(chars)
        {
            int cnum = cJSON_GetArraySize(chars);
            batch->items[i].char_num = cnum;

            if(cnum > 32) cnum = 32;

            for(int j = 0; j < cnum; j++)
            {
                cJSON *ch = cJSON_GetArrayItem(chars, j);

                cJSON *c = cJSON_GetObjectItem(ch, "c");
                cJSON *p = cJSON_GetObjectItem(ch, "conf");

                if(c && c->valuestring)
                    batch->items[i].chars[j].c = c->valuestring[0];

                if(p)
                    batch->items[i].chars[j].conf = p->valuedouble;
            }
        }
    }

    cJSON_Delete(root);
    return batch;
}
