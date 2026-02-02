#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    char input[SHM_SIZE];

    sem_t *mutex = SEM_FAILED;
    sem_t *data = SEM_FAILED;
    FILE *fp = NULL;

    shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid < 0)
        goto cleanup;

    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1)
        goto cleanup;

    mutex = sem_open(MUTEX_SEM, O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED)
        goto cleanup;

    data = sem_open(DATA_SEM, O_CREAT, 0666, 0);
    if (data == SEM_FAILED)
        goto cleanup;

    printf("Enter message: ");
    if (fgets(input, SHM_SIZE, stdin) == NULL)
        goto cleanup;

    input[strcspn(input, "\n")] = '\0';

    sem_wait(mutex);

    strncpy((char *)shared_memory, input, SHM_SIZE);
    ((char *)shared_memory)[SHM_SIZE - 1] = '\0';

    fp = fopen(LOG_FILE, "a");
    
    if (fp) {
        fprintf(fp, "Writer PID %d: %s\n", getpid(), input);
        fclose(fp);
    }

    sem_post(mutex);
    sem_post(data);

cleanup:

    if (data != SEM_FAILED)
        sem_close(data);

    if (mutex != SEM_FAILED)
        sem_close(mutex);

    if (shared_memory != (void *)-1)
        shmdt(shared_memory);

    return 0;
}
