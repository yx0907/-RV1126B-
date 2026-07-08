#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "rti_conf.h"

int rti_ls_en_fd, rti_ls_addr_fd;
const char enable = 1;
const char disable = 0;

void rti_ls_init(void)
{
    rti_ls_addr_fd = open(DRV_ADDR_PATHNAME, O_WRONLY);
    rti_ls_en_fd = open(DRV_EN_PATHNAME, O_WRONLY);
    if(rti_ls_addr_fd == -1 || rti_ls_en_fd == -1)
    {
        perror("LS_DRV_OPEN");
        exit(EXIT_FAILURE);
    }   
}

void turn_all_lights_on(void)
{
    if(write(rti_ls_en_fd, &enable, 1) == -1)
    {
        perror("LS_ON_WRITE");
        exit(EXIT_FAILURE);
    }   
}

void turn_all_lights_off(void)
{
    if(write(rti_ls_en_fd, &disable, 1) == -1)
    {
        perror("LS_OFF_WRITE");
        exit(EXIT_FAILURE);
    }   
}

void select_lights(char * i)
{
    if(write(rti_ls_addr_fd, i, 1) == -1)
    {
        perror("LS_ADDR_WRITE");
        exit(EXIT_FAILURE);
    }   
}

void rti_ls_release(void)
{
    close(rti_ls_addr_fd);
    close(rti_ls_en_fd);
}
