#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "shared.h"

int main(void)
{
  int shmid = -1;
  void *shared_memory = (void *)-1;
  char *message = NULL;
  sem_t *mutex = SEM_FAILED;
  sem_t *data = SEM_FAILED;
  FILE *fp = NULL;



shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
if (shmid < 0)
    goto cleanup;


shared_memory = shmat(shmid, NULL, 0);
if (shared_memory == (void *)-1)
    goto cleanup;


message = (char *)shared_memory;


mutex = sem_open(MUTEX_SEM, O_CREAT, 0666, 1);
if (mutex == SEM_FAILED)
    goto cleanup;


data = sem_open(DATA_SEM, O_CREAT, 0666, 0);
if (data == SEM_FAILED)
    goto cleanup;


sem_wait(data);
sem_wait(mutex);


printf("Reader got -> %s\n", message);


fp = fopen(LOG_FILE, "a");


if (fp)
{
    fprintf(fp, "Reader PID %d: %s\n", getpid(), message);
    fclose(fp);
}


sem_post(mutex);


cleanup : if (data != SEM_FAILED) sem_close(data);
if (mutex != SEM_FAILED)
    sem_close(mutex);
if (shared_memory != (void *)-1)
    shmdt(shared_memory);

return 0;

}
