#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define BUFF_SIZE 100
#define OFFSET 500


int main(int argc, char *argv[])
{
    /* MODE
     * 0: normal
     * 1: fast doubling
     * 2: clz fast doubling
     * 3: bn
     * 4: bn + fast doubling
     */
    int mode = 0;
    if (argc == 2) {
        mode = *argv[1] - '0';
    }


    char buf[BUFF_SIZE];

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= OFFSET; i++) {
        lseek(fd, i, SEEK_SET);
        long long sz = write(fd, buf, mode);
        printf("%lld ", sz);
    }
    close(fd);
    return 0;
}