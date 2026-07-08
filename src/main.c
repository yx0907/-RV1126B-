#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "v4l2_capture.h"
#include "ps.h"
#include "light_source.h"
#include "normal_to_binary.h"
#include "rti_conf.h"

typedef enum
{
    STATE_IDLE,
    STATE_FIND,
    STATE_CAPTURE,
    STATE_RTI,
    STATE_AI,
    STATE_SAVE,
    STATE_FINISH,//unuse
    STATE_NUM//unuse
}lvgl_state;

int main(void)
{
    GImage * input_images = malloc(16 * sizeof(* input_images));
    GImage * output_image = malloc(sizeof(* output_image));
    float * normals = malloc(FRAME_HEIGHT * FRAME_WIDTH * 3 * sizeof(float));
    float * albedo = malloc(FRAME_HEIGHT * FRAME_WIDTH * sizeof(float));
    
    rti_ls_init();
    gamma_tables_init();
    rti_ps_init();

    int io_init_fd = open(IO_INIT_PATHNAME, O_WRONLY);
    if(io_init_fd == -1)
    {
        perror("ioinit_open");
        goto error;
    }
    if(write(io_init_fd, "169", sizeof("169")) == -1)
    {
        perror("ioinit_write");
        goto error;
    }
    if(write(io_init_fd, "170", sizeof("170")) == -1)
    {
        perror("ioinit_write");
        goto error;
    }
    close(io_init_fd);

    int io_dir_fd = open(PASS_IO_DIR_PATHNAME, O_WRONLY);
    if(io_dir_fd == -1)
    {
        perror("iodir_open");
        goto error;
    }
    if(write(io_dir_fd, "out", sizeof("out")) == -1)
    {
        perror("iodir_write");
        goto error;
    }
    close(io_dir_fd);

    io_dir_fd = open(ALARM_IO_DIR_PATHNAME, O_WRONLY);
    if(io_dir_fd == -1)
    {
        perror("iodir_open");
        goto error;
    }
    if(write(io_dir_fd, "out", sizeof("out")) == -1)
    {
        perror("iodir_write");
        goto error;
    }
    close(io_dir_fd);

    int ala_io_fd = open(ALARM_IO_PATHNAME, O_WRONLY);
    if(ala_io_fd == -1)
    {
        perror("ala_io_open");
        goto error;
    }
    if(write(ala_io_fd, "1", sizeof("1")) == -1)
    {
        perror("ala_io_write");
        goto error;
    }
    close(ala_io_fd);

    printf("System init done!\r\n");

    pid_t lvgl_proc_pid;
    int lvgl_pipe_fd[2];
    int sys_state;
    int fd;

    if(pipe(lvgl_pipe_fd) < 0)
    {
        perror("lvgl_pipe");
        goto error;
    }

    lvgl_proc_pid = fork();
    if(lvgl_proc_pid < 0)
    {
        perror("lvgl_fork");
        goto error;
    }
    else if(lvgl_proc_pid == 0)
    {
        close(lvgl_pipe_fd[1]);//Close write
        char pipe_read_fd[16];
        sprintf(pipe_read_fd, "%d", lvgl_pipe_fd[0]);
        if(execl(LVGL_EXEC_PATHNAME, "LVGL", pipe_read_fd/*Only can read*/, NULL) < 0)
        {
            perror("lvgl_exec");//Wish to never be here
            goto error;
        }
    }
    else
    {
        //Parent process
        close(lvgl_pipe_fd[0]);//Close read
        sys_state = STATE_IDLE;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }
    }
    
    //Main process
    int pass_io_fd;
    int ret;
    
    while(1)
    {
        sys_state = STATE_IDLE;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }

        char led_idx = 6;
        select_lights(&led_idx);
        turn_all_lights_on();
        if(system(FINDER_COMMAND) != 0)
        {
            printf("Fail to load character finder!\r\n");
            goto error;
        }
        else
        {
            sys_state = STATE_FIND;
            if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
            {
                perror("state_write");
                goto error;
            }
            turn_all_lights_off();
            //TODO if need to cut image
        }

        struct rti_cam_desc * desc = rti_camera_init();
        sys_state = STATE_CAPTURE;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }
        ret = get_16_grayscale_images(input_images, desc);
        if(ret != 0)
        {
            printf("Fail to get images, the index is %u\r\n", ret - 1);
            rti_camera_release(desc);
            goto error;
        }
        else
        {
            rti_camera_release(desc);
        }

#ifdef CALIBRATE_LS_VECTOR
        /*Using this port to calibrate the light source vectors*/
        char input_image_name[10];
        for(int a = 0; a < 16; a++)
        {
            sprintf(input_image_name, "Image-%u", a);
            int image_fd = open(input_image_name, O_CREAT | O_TRUNC | O_RDWR, 0644);
            write(image_fd, &input_images[a], sizeof(* input_images));
            close(image_fd);
        }
#endif

        sys_state = STATE_RTI;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }
        photometric_stereo(input_images, normals, albedo);
        if(normal_to_binary(normals, FRAME_WIDTH, FRAME_HEIGHT, &output_image->P[0][0]) == -1)
        {
            printf("Fail to convert normals to image!\r\n");
            goto error;
        }
        
        fd = open("HCimage.gray", O_CREAT | O_TRUNC | O_RDWR, 0644);
        if(fd == -1)
        {
            perror("HCimage_open");
            goto error;
        }

        if(write(fd, output_image, sizeof(* output_image)) == -1)
        {
            perror("HCimage_write");
            goto error;
        }
        close(fd);

        sys_state = STATE_AI;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }
        int ocr_ret = system(OCR_COMMAND);
        if(ocr_ret != 0)
        {
            printf("Fail to load OCR %d!\r\n", ocr_ret);
            goto error;
        }
        
        sys_state = STATE_SAVE;
        if(write(lvgl_pipe_fd[1], &sys_state, sizeof(sys_state)) == -1)
        {
            perror("state_write");
            goto error;
        }

        usleep(70000);

        pass_io_fd = open(PASS_IO_PATHNAME, O_WRONLY);
        if(pass_io_fd == -1)
        {
            perror("io_open");
            goto error;
        }
        if(write(pass_io_fd, "0", sizeof("0")) == -1)
        {
            perror("pass_io_write");
            goto error;
        }
        close(pass_io_fd);
    }

error:
    printf("Parent proc exit\r\n");
    close(fd);
    close(ala_io_fd);
    close(pass_io_fd);
    close(io_dir_fd);
    close(io_init_fd);
    turn_all_lights_off();
    rti_ls_release();

    free(input_images);
    free(output_image);
    free(normals);
    free(albedo);

    kill(lvgl_proc_pid, SIGKILL);

    return -1;
}
