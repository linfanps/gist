/**
 * description : 使用消息队列方式进行进程间通信,该进程负责读
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

void get_mq_attr(mqd_t mqid, struct mq_attr *attr)
{
    if (mq_getattr(mqid, attr) < 0) {
        perror("mq_getattr failed\n");
        return ;
    }
    printf("mq_flags=%ld, mq_maxmsg=%ld, mq_msgsize=%ld, mq_curmsgs=%ld\n", attr->mq_flags, attr->mq_maxmsg, attr->mq_msgsize, attr->mq_curmsgs);
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
    struct mq_attr attr;
    get_mq_attr(mqid, &attr);
    char *msg = malloc(attr.mq_msgsize);
    while (mq_receive(mqid, msg, attr.mq_msgsize, NULL) > 0) {
        printf("receive %s\n", msg); 
        get_mq_attr(mqid, &attr); 
        printf("sleeping...\n");
        sleep(1);
    }
    
    mq_close(mqid);
    return 0;
}

