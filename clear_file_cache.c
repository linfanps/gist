#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct stat st;
    int fd;

    if (argc < 2) {
        printf("please specify the filename\n");
        return -1;
    }
    
    if (stat(argv[1], &st) < 0) {
        printf("stat file %s failed\n", argv[1]);
        return -1;
    }

    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        printf("open %s failed\n", argv[1]);
        return -1;
    }

    if (posix_fadvise(fd, 0, st.st_size, POSIX_FADV_DONTNEED) != 0) {
        printf("posix_fadvise failed\n");
    } else {
        printf("posix_fadvise success\n");
    }
    return 0;
}

