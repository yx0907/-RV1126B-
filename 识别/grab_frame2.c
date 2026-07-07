/*
 * grab_frame.c
 * 从 /dev/video23 取一帧，直接取 Y 平面（灰度）写入 ./preview.gray
 * 文件格式：无头部，直接是 H*W 字节的灰度数据
 * Python 端：
 *   data = open('preview.gray','rb').read()
 *   img  = np.frombuffer(data, np.uint8).reshape((H, W))
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

/* ── 配置 ── */
#define CAMERA_PATH   "/dev/video31"
#define OUTPUT_PATH   "./preview.gray"
#define CAP_W         1920
#define CAP_H         1080
#define BUF_COUNT     4
#define MAX_PLANES    2
#define DROP_FRAMES   4     /* 丢弃前 N 帧，等曝光稳定 */

int main(void)
{
    int fd = open(CAMERA_PATH, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    /* 设置格式 */
    struct v4l2_format fmt = {0};
    fmt.type                   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width       = CAP_W;
    fmt.fmt.pix_mp.height      = CAP_H;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
    fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.num_planes  = 2;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) { perror("VIDIOC_S_FMT"); goto err; }

    unsigned int num_planes = fmt.fmt.pix_mp.num_planes;

    /* 申请缓冲区 */
    struct v4l2_requestbuffers req = {0};
    req.count  = BUF_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) { perror("VIDIOC_REQBUFS"); goto err; }

    /* mmap */
    void *buf_start[BUF_COUNT][MAX_PLANES];
    size_t buf_len[BUF_COUNT][MAX_PLANES];

    for (int i = 0; i < (int)req.count; i++) {
        struct v4l2_buffer buf   = {0};
        struct v4l2_plane planes[MAX_PLANES];
        memset(planes, 0, sizeof(planes));
        buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory   = V4L2_MEMORY_MMAP;
        buf.index    = i;
        buf.length   = num_planes;
        buf.m.planes = planes;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) { perror("VIDIOC_QUERYBUF"); goto err; }

        for (int j = 0; j < (int)num_planes; j++) {
            buf_len[i][j]   = planes[j].length;
            buf_start[i][j] = mmap(NULL, planes[j].length,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   fd, planes[j].m.mem_offset);
            if (buf_start[i][j] == MAP_FAILED) { perror("mmap"); goto err; }
        }
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) { perror("VIDIOC_QBUF"); goto err; }
    }

    /* 开流 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) { perror("VIDIOC_STREAMON"); goto err; }

    /* 丢弃前 DROP_FRAMES 帧 */
    for (int d = 0; d < DROP_FRAMES; d++) {
        struct v4l2_buffer buf   = {0};
        struct v4l2_plane planes[MAX_PLANES];
        memset(planes, 0, sizeof(planes));
        buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory   = V4L2_MEMORY_MMAP;
        buf.length   = num_planes;
        buf.m.planes = planes;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) { perror("VIDIOC_DQBUF drop"); goto err; }
        if (ioctl(fd, VIDIOC_QBUF,  &buf) < 0) { perror("VIDIOC_QBUF  drop"); goto err; }
    }

    /* 取一帧 */
    struct v4l2_buffer buf   = {0};
    struct v4l2_plane  planes[MAX_PLANES];
    memset(planes, 0, sizeof(planes));
    buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory   = V4L2_MEMORY_MMAP;
    buf.length   = num_planes;
    buf.m.planes = planes;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) { perror("VIDIOC_DQBUF"); goto err; }

    /* 直接写 Y 平面（灰度），不做色彩转换 */
    FILE *fp = fopen(OUTPUT_PATH, "wb");
    if (!fp) { perror("fopen output"); goto err; }
    fwrite(buf_start[buf.index][0], 1, CAP_W * CAP_H, fp);
    fclose(fp);
    printf("saved %s  (%dx%d GRAY)\n", OUTPUT_PATH, CAP_W, CAP_H);

    /* 关流 */
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < (int)req.count; i++)
        for (int j = 0; j < (int)num_planes; j++)
            munmap(buf_start[i][j], buf_len[i][j]);
    close(fd);
    return 0;

err:
    close(fd);
    return 1;
}
