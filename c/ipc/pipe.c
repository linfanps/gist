/**
 * description : 父子进程使用管道进行进程通信，父进程向子进程发送文件路径名，子进程根据路径名读取文件内容并发送给主进程
 *               创建2个pipe，一个用来读，一个用来写
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
    int pipe1[2], pipe2[2];
    pid_t cpid;

    if (pipe(pipe1) < 0) {
        perror("can not create pipe1\n");
        return -1;
    }
    if (pipe(pipe2) < 0) {
        perror("can not create pipe2\n");
        close(pipe1[0]);
        close(pipe1[1]);
        return -1;
    }
    
    if ((cpid = fork()) == 0) {
        //child process
        close(pipe1[1]);
        close(pipe2[0]);
        server(pipe1[0], pipe2[1]);
        return 0;
    } else if (cpid > 0) {
        close(pipe1[0]);
        close(pipe2[1]);
        client(pipe2[0], pipe1[1]);

        waitpid(cpid, NULL, 0);
    }else {
        perror("fork failed\n");
        return -1;
    }
    
    return 0;
}

