/**
 * description : 使用消息队列方式进行进程间通信,该进程负责写
 * author : linfan
 * date : 2014-12-03
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MSGLEN 10
void get_mq_attr(mqd_t mqid)
{
    struct mq_attr attr;
    if (mq_getattr(mqid, &attr) < 0) {
        perror("mq_getattr failed\n");
        return ;
    }
    printf("mq_flags=%ld, mq_maxmsg=%ld, mq_msgsize=%ld, mq_curmsgs=%ld\n", attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}

int main(int argc, char **argv)
{
    mqd_t mqid;
    if ((mqid = mq_open("/anonymous.mq", O_RDWR | O_CREAT , 0666, NULL)) < 0) {
        if (errno == EEXIST) {
            printf("file exist, unlink...\n");
            mq_unlink("/anonymous.mq");
        } else {
            perror("mq_open failed");
        }
        return -1;
    }
    char msg[MSGLEN];
    int i;
    for (i = 1; i <= 5; i++) {
        snprintf(msg, MSGLEN, "msg %d", i);
        if (mq_send(mqid, msg, strlen(msg), i) < 0) {
            perror("mq_send failed");
        } else {
            printf("send msg %s\n", msg);
        }
       get_mq_attr(mqid); 
    }
    mq_close(mqid);
    return 0;
}

