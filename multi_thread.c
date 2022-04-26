#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS 3
#define FIB_DEV "/dev/fibonacci"


void *perform_work(void *arguments)
{
    int fd = open(FIB_DEV, O_RDWR);
    while (fd < 0) {
        printf("Process %d Fail to open device\n", *((int *) arguments));
        sleep(0.5);
        fd = open(FIB_DEV, O_RDWR);
    }

    char buf[1];
    for (int i = 5; i <= 8; i++) {
        lseek(fd, i, SEEK_SET);
        long long sz = read(fd, buf, 0);
        printf("process %d: f(%d) = %lld\n", *((int *) arguments), i, sz);
        sleep(0.1);
    }
    close(fd);
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];
    int i;
    int result_code;

    // create all threads one by one
    for (i = 0; i < NUM_THREADS; i++) {
        printf("IN MAIN: Creating thread %d.\n", i);
        thread_args[i] = i;
        result_code =
            pthread_create(&threads[i], NULL, perform_work, &thread_args[i]);
        assert(!result_code);
    }

    printf("IN MAIN: All threads are created.\n");

    // wait for each thread to complete
    for (i = 0; i < NUM_THREADS; i++) {
        result_code = pthread_join(threads[i], NULL);
        assert(!result_code);
        printf("IN MAIN: Thread %d has ended.\n", i);
    }

    printf("MAIN program has ended.\n");
    return 0;
}