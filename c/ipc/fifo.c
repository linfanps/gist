/**
 * description : FIFO 父子进程使用管道进行进程通信，父进程向子进程发送文件路径名，子进程根据路径名读取文件内容并发送给主进程
 *               这里为了方便，还是在父子进程之间采用FIFO进行通信，FIFO可以应用在非亲缘进程之间通信
 * author : linfan
 * date : 2014-12-03
 */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 128
#define FILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FIFO1 "/tmp/fifo.1"
#define FIFO2 "/tmp/fifo.2"
void client(int readfd, int writefd)
{
    char *filename = "file.txt";
    size_t len = strlen(filename);
    char buff[MAXLEN];
    int n;
    if (write(writefd, filename, len) < 0) {
        perror("main process:write filename failed\n");
        return ;
    }
    printf("main process:receive %s content\n", filename);
    while ((n = read(readfd, buff, MAXLEN)) > 0) {
        write(STDOUT_FILENO, buff, n);
    }
}

void server(int readfd, int writefd)
{
    size_t n;
    int fd;
    char buff[MAXLEN];

    if ((n = read(readfd, buff, MAXLEN)) <= 0) {
        perror("child process:read filename error\n");
        return ;
    }
    buff[n] = '\0';
    printf("child process:\n");
    if ((fd = open(buff, O_RDONLY)) < 0) {
        perror("child process:open file failed\n");
        snprintf(buff + n, sizeof(buff) - n, "can not open %s\n", strerror(errno));
        n = strlen(buff);
        write(writefd, buff, n);
    } else {
        printf("child process: open file %s success\n", buff);
        while ((n = read(fd, buff, MAXLEN)) > 0) {
            write(writefd, buff, n);
        }
        close(fd);
    }
}
int main(int argc, char **argv)
{
    int writefd, readfd;
    pid_t cpid;
    
    if (mkfifo(FIFO1, FILEMODE) < 0 && errno != EEXIST) {
        perror("create fifo1 failed\n");
        return -1;
    }
    if (mkfifo(FIFO2, FILEMODE) < 0 && errno != EEXIST) {
        perror("create fifo2 failed\n");
        unlink(FIFO1);
        return -1;
    }
    /*
     * 如果当前尚未有任何进程打开某个FIFO来写，那么打开FIFO来读的进程将阻塞
     * 如果当前尚未有任何进程打开某个FIFO来读，那么打开FIFO来写的进程讲阻塞
     * 如果下面的父子进程都选择打开同一个FIFO来读，则发生死锁
     */
    if ((cpid = fork()) == 0) {
        //child process
        readfd = open(FIFO1, O_RDONLY, 0);
        writefd = open(FIFO2, O_WRONLY, 0);
        server(readfd, writefd);
        return 0;
    } else if (cpid > 0) {
        //readfd = open(FIFO2, O_RDONLY, 0);产生死锁
        writefd = open(FIFO1, O_WRONLY, 0);
        readfd = open(FIFO2, O_RDONLY, 0);
        client(readfd, writefd);

        waitpid(cpid, NULL, 0);
    }else {
        perror("fork failed\n");
        return -1;
    }
    
    return 0;
}

