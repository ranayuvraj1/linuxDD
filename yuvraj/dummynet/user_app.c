#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define IOC_SEND _IOW('H',1,struct ioc_req)

struct ioc_req {
    int if_id;
    int len;
};

int fd;

void *eth0_thread(void *arg)
{
    struct ioc_req r;
    while(1){
        r.if_id=0;
        r.len=rand()%1024+1;
        ioctl(fd,IOC_SEND,&r);
        sleep(2);
    }
}

void *eth1_thread(void *arg)
{
    struct ioc_req r;
    while(1){
        r.if_id=1;
        r.len=rand()%1024+1;
        ioctl(fd,IOC_SEND,&r);
        sleep(5);
    }
}

int main()
{
    pthread_t t0,t1;

    fd=open("/dev/hyd",O_RDWR);

    pthread_create(&t0,NULL,eth0_thread,NULL);
    pthread_create(&t1,NULL,eth1_thread,NULL);

    pthread_join(t0,NULL);
    pthread_join(t1,NULL);
}
