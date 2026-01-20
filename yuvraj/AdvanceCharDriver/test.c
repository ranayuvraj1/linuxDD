#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MY_MAGIC 'M'
#define IOCTL_GET_VAL _IOR(MY_MAGIC, 1, int)
#define IOCTL_SET_VAL _IOW(MY_MAGIC, 2, int)
#define IOCTL_CLR_VAL _IO(MY_MAGIC, 3)

int main()
{
    int fd, val;

    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    val = 100;
    ioctl(fd, IOCTL_SET_VAL, &val);

    val = 0;
    ioctl(fd, IOCTL_GET_VAL, &val);
    printf("GET_VAL = %d\n", val);

    ioctl(fd, IOCTL_CLR_VAL);

    close(fd);
    return 0;
}
