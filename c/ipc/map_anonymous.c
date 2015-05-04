/**
 * description : mmap使用，父子进程通过匿名映射实现共享内存
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
    int i;
    people *p_map;
    char temp;

    p_map = (people*)mmap(NULL, sizeof(people) * 10, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (p_map == MAP_FAILED) {
        printf("mmap failed\n");
    }

    if (fork() == 0) {
        sleep(2);
        for (i = 0; i < 5; i++) {
            printf("child process name:%s, age:%d\n", p_map[i].name, p_map[i].age);
        }
        p_map[0].age = 100;
        return 0;
    } else {
        for (i = 0; i < 5; i++) {
            snprintf(p_map[i].name, 4, "%d", i);
            p_map[i].age = 20 + i;
        }
        sleep(5);
        printf("parent process the first people name %s age %d\n", p_map[0].name, p_map[0].age);
        munmap(p_map, sizeof(people)*10);
        printf("umap ok\n");
    }
    return 0;
}

