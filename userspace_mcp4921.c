#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "mcp4921_commands.h"

int main()
{
        int fd;
        int arg;
        char buff[] = "1000";
 
        fd = open("/dev/mcp4921", O_RDWR);
        if (fd < 0) {
                printf("Cannot open device file.\n");
                return fd;
        }

        printf("Driver open\n");

        ioctl(fd, MCP4921_RESET, 0); 

        arg = 1;
        ioctl(fd, MCP4921_ENABLE, &arg);
        ioctl(fd, MCP4921_GAIN, &arg);
        arg = 0;
        ioctl(fd, MCP4921_VREF_BUFF, &arg);

        write(fd, buff, sizeof(buff));
 
        printf("Driver close\n");
        close(fd);

        return 0;
}