/**
 * description : mmap使用，访问共享内存区大小与映射的文件大小关系
 * author : linfan
 * date : 2014-12-03
 */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char **argv)
{
    int fd, i;
    size_t filesize, mmapsize, pagesize, maxsize;
    char *ptr;

    filesize = atoi(argv[2]);
    mmapsize = atoi(argv[3]);
    pagesize = sysconf(_SC_PAGESIZE);
    maxsize = (filesize >= mmapsize) ? filesize : mmapsize;
    
    fd = open(argv[1], O_CREAT | O_RDWR | O_TRUNC, 00777);
    if (fd < 0) {
        printf("open file %s failed\n", argv[1]);
        return -1;
    }

    lseek(fd, filesize - 1, SEEK_SET);
    if (write(fd, "", 1) < 0) {
        printf("write failed\n");
        close(fd);
        return -1;
    }
    ptr = (char*)mmap(NULL, mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    printf("pagesize=%ld\n", (long)pagesize);
    for (i = 0; i < maxsize; i += pagesize) {
        printf("ptr[%d] = %d\n", i, ptr[i]);
        ptr[i] = 1;
        printf("ptr[%d] = %d\n", i + pagesize - 1, ptr[i + pagesize -1]);
        ptr[i + pagesize -1] = 1;
    }

    printf("ptr[%d] = %d\n", i, ptr[i]);
    return 0;
}

