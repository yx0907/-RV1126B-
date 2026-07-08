#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "rti_conf.h"
#include "v4l2_capture.h"
#include "ps.h"
#include "light_source.h"

struct rti_cam_desc * rti_camera_init(void)
{
    struct rti_cam_desc * desc = malloc(sizeof(* desc));

    //Open device
    int fd = open(CAMERA_PATHNAME, O_RDWR);
    if(fd < 0)
    {
        perror("CAM_OPEN");
        goto error;
    } 

    //Inquire device capability
    struct v4l2_capability cap;
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("VIDIOC_QUERYCAP");
        goto error;
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) 
    {
        printf("Not a video capture multiplanar device!\r\n");
        goto error;
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) 
    {
        printf("Not support streaming I/O!\r\n");
        goto error;
    }

    //Set format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix.width = FRAME_WIDTH;
    fmt.fmt.pix.height = FRAME_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12M;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.num_planes = 2;
    if(ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("VIDIOC_S_FMT");
        goto error;
    }   
    if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_NV12M) 
    {
        printf("Not support NV12M!\r\n");
        goto error;
    }
    unsigned int num_planes = fmt.fmt.pix_mp.num_planes;
    if(num_planes > MAX_PLANES)
    {
        printf("The number of planes is over!\r\n");
        goto error;
    }
    printf("Size: %ux%u, Format: %.4s\r\n",
           fmt.fmt.pix.width, fmt.fmt.pix.height,
           (char *)&fmt.fmt.pix.pixelformat);

    //Request buffers
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = RTI_V4L2_BUF_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("VIDIOC_REQBUFS");
        goto error;
    }
    if(req.count != RTI_V4L2_BUF_COUNT) 
    {
        printf("Not enough memory!\r\n");
        goto error;
    }

    //Remap buffers to user space
    for(int i = 0; i < req.count; i++) 
    {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(struct v4l2_plane) * num_planes);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = num_planes;
        buf.m.planes = planes;
        if(ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            perror("VIDIOC_QUERYBUF");
            goto error;
        }
        for(int j = 0; j < num_planes; j++)
        {   
            desc->usr_buf_length[i][j] = planes[j].length;
            desc->usr_buf_start[i][j] = mmap(NULL, planes[j].length,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED, fd,
                                             planes[j].m.mem_offset);
            if(desc->usr_buf_start[i][j] == MAP_FAILED)
            {
                printf("Fail to mmap!\r\n");
                goto mmap_error;
            }   
        } 
        //Put buffer in queue
        if(ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("VIDIOC_QBUF");
            goto mmap_error;
        }
    }

    //Turn video stream on
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("VIDIOC_STREAMON");
        goto mmap_error;
    }

    //Done
    desc->fd = fd;
    desc->num_planes = num_planes;
    printf("Camera init succeed!\r\n");
    return desc;

mmap_error:
    for(int i = 0; i < req.count; i++)
        for(int j = 0; j < num_planes; j++)
            munmap(desc->usr_buf_start[i][j], desc->usr_buf_length[i][j]);

error:
    close(fd);
    free(desc);
    exit(EXIT_FAILURE);
}

void rti_camera_release(struct rti_cam_desc * desc)
{
    //Turn video stream off
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(desc->fd, VIDIOC_STREAMOFF, &type) < 0)
        printf("Fail to turn video stream off!\r\n");
    //Release the resources
    for(int i = 0; i < RTI_V4L2_BUF_COUNT; i++)
        for(int j = 0; j < desc->num_planes; j++)
            munmap(desc->usr_buf_start[i][j], desc->usr_buf_length[i][j]);
    close(desc->fd);
    free(desc);
}

static inline int rti_cam_get_frame(struct rti_cam_desc * desc, struct v4l2_buffer * buf_info,
                                    struct v4l2_plane * plane_info)
{
    int ret;
    int dropped = 0;
    int flags;
    int fd = desc->fd;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[desc->num_planes];

    flags = fcntl(fd, F_GETFL, 0);//Save the flags of camera file (blocking mode)
    if(flags == -1) 
        return -1;
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)//Switch to NON-blocking mode
        return -1;

    //Drop old frames
    while(dropped < MAX_DROP) 
    {
        memset(&buf, 0, sizeof(buf));
        memset(planes, 0, sizeof(struct v4l2_plane) * desc->num_planes);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = desc->num_planes;
        buf.m.planes = planes;

        ret = ioctl(fd, VIDIOC_DQBUF, &buf);
        if(ret == 0) 
        {
            ioctl(fd, VIDIOC_QBUF, &buf);
            dropped++;
        } 
        else 
        {
            if(errno == EAGAIN) 
            {
                //break;
            } 
            else 
            {
                printf("Fail to drop old frames!\r\n");
                fcntl(fd, F_SETFL, flags);//Recover
                return -1;
            }
        }
    }

    //Get a new frame
    fcntl(fd, F_SETFL, flags);//Recover
    memset(buf_info, 0, sizeof(* buf_info));
    memset(plane_info, 0, sizeof(struct v4l2_plane) * desc->num_planes);
    buf_info->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf_info->memory = V4L2_MEMORY_MMAP;
    buf_info->length = desc->num_planes;
    buf_info->m.planes = plane_info;
    ret = ioctl(fd, VIDIOC_DQBUF, buf_info);
    if(ret < 0) 
    {
        printf("Fail to get a new frame!\r\n");
        return -1;
    }

    //Done
    return 0;
}

static inline int rti_cam_return_buf(struct rti_cam_desc * desc, struct v4l2_buffer * buf_info)
{
    if(ioctl(desc->fd, VIDIOC_QBUF, buf_info) < 0)
    {
        printf("Fail to return buffer!\r\n");
        return -1;
    }
    else
        return 0;
}

/*
*   Done: 0
*   Error: 1-16
*/
int get_16_grayscale_images(GImage * images, struct rti_cam_desc * desc)
{
    struct v4l2_buffer buf_info;
    struct v4l2_plane planes[MAX_PLANES];
    
    for(char i = 0; i < 16; i++)
    {
        turn_all_lights_off();
        select_lights(&i);
        turn_all_lights_on();
        if(rti_cam_get_frame(desc, &buf_info, planes) == -1)
            return i + 1;
        memcpy(&images[i], desc->usr_buf_start[buf_info.index][0], planes[0].bytesused);
        if(rti_cam_return_buf(desc, &buf_info) == -1)
            return i + 1;
    }
    turn_all_lights_off();

    return 0;
}
