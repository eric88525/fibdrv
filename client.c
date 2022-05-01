#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define BUFF_SIZE 100


/* NUM_MODE
 * 0 - normal
 * 1 - bn
 * 2- teacher's bn
 */
#define NUM_MODE 2

int main()
{
    long long sz;
    char buf[BUFF_SIZE] = {0};
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, 999);
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, NUM_MODE);
        if (NUM_MODE == 0) {
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%lld.\n",
                   i, sz);
        } else {
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, buf);
        }
    }

    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, NUM_MODE);

        if (NUM_MODE == 0) {
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%lld.\n",
                   i, sz);
        } else {
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%s.\n",
                   i, buf);
        }
    }

    close(fd);
    return 0;
}
