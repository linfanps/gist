#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define EPOLL_SIZE 1000
#define MAX_EVENTS 10

/*
 * struct epoll_event 
 * {
 *  uint32_t events;
 *  epoll_data_t data;
 * }
 *
 * typedef union epoll_data 
 * {
 *  void *ptr;
 *  int fd;
 *  uint32_t u32;
 *  uint64_t u64;
 * } epoll_data_t
 */

int create_socket()
{
    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    return fd;
}
int set_fd_nonblock(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL,0)) == -1) {
        perror("F_GETFL error");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("F_SETFL error");
        return -1;
    }

    return 0;
}
int main(int argc, char **argv)
{
    int efd, sfd;
    struct epoll_event event;
    struct epoll_event *events;
    struct sockaddr_in addr;
    if ((efd = epoll_create(EPOLL_SIZE)) == -1 ) {
        printf("memory is not enough\n");
        exit(-1);
    }
    if ((sfd = create_socket()) == -1 ) {
        printf("create socket failed");
        close(efd);
        exit(-1);
    }
    /*
    if (set_fd_nonblock(sfd) == -1) {
        printf("set fd nonblock failed\n");
        close(efd);
        close(sfd);
        exit(-1);
    }

    */
    connect(sfd, (struct sockaddr*)&addr, sizeof(addr));
    event.data.fd = sfd;
    event.events = EPOLLIN  | EPOLLERR;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) == -1) {
        printf("epoll_ctl failed\n");
        close(efd);
        close(sfd);
        exit(-1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1111);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*10);
    
    while (1) {
        int n, i;
        n = epoll_wait(efd, events, MAX_EVENTS, -1);
        for (i = 0; i < n; i++) {
            if (events[i].events & EPOLLIN) {
                printf("%d:EPOLLIN\n", events[i].data.fd);
            } 
            if (events[i].events & EPOLLHUP) { /* 每次有未连接的socket被加入监听列队时，会触发EPOLLHUP 
                                                比如socket创建了套接字以后未调用connect或未调用listen，就直接加入epoll监听列队 
                                                对端发送RST包会触发EPOLLHUP事件 */
                printf("%d:EPOLLHUP\n", events[i].data.fd);
            } 
            if (events[i].events & EPOLLERR) { /* 读写时发现对方异常如掉线等，触发EPOLLERR事件 */
                printf("%d:EPOLLERR\n", events[i].data.fd);
            }
            if (events[i].events & EPOLLOUT) {
                printf("%d:EPOLLOUT\n", events[i].data.fd);
            }
        }
    }
    return 0;
}

