#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "receive/fifot.h"

int fifot() {
    const char *fifo_path = "/home/elf/my_fifo";

    // 确保 FIFO 存在（若不存在则创建）
    if (mkfifo(fifo_path, 0666) == -1) {
        struct stat st;
        if (stat(fifo_path, &st) != 0 || !S_ISFIFO(st.st_mode)) {
            perror("mkfifo");
            return -1;
        }
    }

    // 尝试以非阻塞只写方式打开，若没有读端则直接退出
    int fd = open(fifo_path, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        if (errno == ENXIO) {
            printf("[FIFO] No reader, skip sending.\n");
            return -1;
        } else {
            perror("open");
            exit(EXIT_FAILURE);
        }
    }

    // 读端存在，写入数字 1（带换行，方便 Python 按行读取）
    dprintf(fd, "1\n");
    close(fd);
    return 0;
}
