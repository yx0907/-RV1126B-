#include "ai_comm.h"
#include "core/state_machine.h"
#include "ui/ui_state.h"
#include "external/cjson/cJSON.h"
#include "lvgl/lvgl.h"
#define PORT 9000
static pthread_t ai_thread;
static int server_fd = -1;
static int client_fd = -1;
static void handle_ai_message(const char *json);
int ui_ready = 0;
typedef struct {
    int state;
} detector_msg_t;

static void detector_async_cb(void *data)
{
    detector_msg_t *msg = (detector_msg_t *)data;

    ui_state_set_hakken_state(msg->state);
    state_machine_set(STATE_HAKKEN, 0);

    free(msg);
}

static void ocr_async_cb(void *data)
{
    ocr_batch_t *batch = (ocr_batch_t *)data;

    ocr_process_batch(batch);

    // 如果 ocr_process_batch 没释放，就在这里 free
    free(batch);
}


typedef struct {
    system_state_t state;
    int error_flag;
    system_state_t error_state;
} ui_update_t;
#define AI_PORT 9000
int Internetlink=0;
static int recv_all(int fd, char *buf, int len)
{
    int total = 0;

    while(total < len)
    {
        
        int n = recv(fd, buf + total, len - total, 0);
        Internetlink = n;
        if(n <= 0)
            return -1;

        total += n;
    }

    return total;
}

static void *ai_server_thread(void *arg) 
{ 
    printf("[AI] thread enter\n"); 
    printf("[AI] socket=%d\n", server_fd); 
    struct sockaddr_in addr; 
    socklen_t addrlen = sizeof(addr); 
    server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_fd < 0) 
    { 
      perror("socket"); return NULL; 
    } 
    int opt = 1; 
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
    addr.sin_family = AF_INET; addr.sin_port = htons(9000); 
    addr.sin_addr.s_addr = INADDR_ANY; 
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
        { 
            perror("bind"); 
            close(server_fd); return NULL;
        } 
    if (listen(server_fd, 1) < 0) 
        { 
            perror("listen"); 
            close(server_fd); 
            return NULL; 
        } 
    printf("AI server listening on port %d...\n", PORT); 
    while (1) 
        { 
            // 等待客户端连接
            printf("[AI] waiting for client...\n");
            client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen); 
            if (client_fd < 0) { perror("accept"); 
            sleep(1); 
            continue; 
            } 
        
    printf("[AI] client connected\n"); 
    Internetlink = 1; 
    // 标记网络已连接 
    // 消息接收循环 
    while (1) 
        { 
            uint32_t msg_len = 0; 
            if (recv_all(client_fd, (char*)&msg_len, 4) <= 0) 
                { 
                    printf("[AI] client disconnected (recv len)\n"); 
                    break; 
                    // 连接断开，退出接收循环 
                } 
              msg_len = ntohl(msg_len); 
              if (msg_len == 0 || msg_len > 100000) 
                  { 
                      printf("[AI] invalid msg_len: %u\n", msg_len); 
                      continue; 
                      // 继续接收下一条消息 
                  } 
              char *buffer = malloc(msg_len + 1); 
              if (!buffer) 
                  { 
                      printf("[AI] malloc failed\n"); continue;
                  } 
              if (recv_all(client_fd, buffer, msg_len) <= 0) 
                  { 
                      printf("[AI] client disconnected (recv body)\n"); 
                      free(buffer); 
                      break; 
                      // 连接断开 
                  } 
                  buffer[msg_len] = '\0'; 
            printf("[AI] json received (%u bytes)\n", msg_len);

            handle_ai_message(buffer);

            free(buffer); 
          } 
          // 客户端断开，清理并准备接受新连接 
          close(client_fd); 
          client_fd = -1; 
          Internetlink = 0; 
          printf("[AI] client disconnected, waiting for reconnect...\n"); 
    }
  close(server_fd); return NULL; 
}




static void *ai_server_thread(void *arg);

void ai_comm_init(void)
{
    printf("[AI] init...\n");
    int ret = pthread_create(
            &ai_thread,
            NULL,
            ai_server_thread,
            NULL);

    printf("[AI] pthread_create ret=%d\n", ret);

    if(ret != 0)
    {
        printf("[AI] pthread_create failed\n");
        return;
    }

    printf("[AI] thread created\n");
}

static void handle_ai_message(const char *json)
{
    if (!ui_ready) return;

    cJSON *root = cJSON_Parse(json);
    if(!root)
    {
        printf("[AI] JSON parse failed\n");
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");

    /* =========================
     * 1. detector
     * ========================= */
    if(type && type->valuestring &&
       strcmp(type->valuestring, "detector") == 0)
    {
        cJSON *state = cJSON_GetObjectItem(root, "state");

        if(state)
        {
            detector_msg_t *msg = malloc(sizeof(detector_msg_t));
            if(msg)
            {
                msg->state = state->valueint;   // ✔ 修复字段错误

                lv_async_call(detector_async_cb, msg);
            }
        }

        cJSON_Delete(root);
        return;
    }

    /* =========================
     * 2. OCR
     * ========================= */
    cJSON *items = cJSON_GetObjectItem(root, "items");

    if(items)
    {
        ocr_batch_t *batch = parse_json_to_batch(json);

        if(batch)
        {
            printf("[OCR] batch ready\n");

            /* 必须保证 batch 是完全独立内存 */
            lv_async_call(ocr_async_cb, batch);
        }
    }

    cJSON_Delete(root);
}
