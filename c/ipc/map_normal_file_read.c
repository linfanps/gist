/**
 * description : mmap使用，两个进程通过映射普通文件实现共享内存通信，本程序进行读操作
 * author : linfan
 * date : 2014-12-02
 */
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


typedef struct people {
    char name[4];
    int age;
} people;

int main(int argc, char **argv)
{
    int fd, i;
    people *p_map;

    fd = open(argv[1], O_CREAT | O_RDWR, 00777);
    if (fd < 0) {
        printf("open file %s failed\n", argv[1]);
        return -1;
    }
    p_map = (people*)mmap(NULL, 10*sizeof(people), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    for (i = 0; i < 10; i++) {
        printf("name %s,age:%d\n", p_map[i].name, p_map[i].age);
    }

    munmap(p_map, sizeof(people) * 10);
    return 0;
}

