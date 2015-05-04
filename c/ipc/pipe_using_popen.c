/**
 * description : 使用系统提供的popen函数，创建一个管道并启动shell进程用以执行函数中指定的command,shell进程要么从管道中读取标准输入，要么往管道中写标准输出(标准错误输出不处理)
 *               函数原型为FILE* popen(const char *command, const char *type), type为r时，调用进程从管道中读取shell进程的标准输出，为w调用进程写到shell进程的标准输入
 * author : linfan
 * date : 2014-12-04
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAXLEN 256
int main(int argc, char **argv)
{
    size_t n;
    char buff[MAXLEN];
    FILE *fp;
    if ((fp = popen("cat /etc/passwd", "r")) == NULL) {
        printf("fopen failed\n");
        return -1;
    }
    while (fgets(buff, MAXLEN, fp) != NULL) {
        fputs(buff, stdout);
    }

    if (pclose(fp) < 0) {
        perror("close fp failed\n");
    }
    return 0;
}

