/**
 * description : 
 * author : linfan
 * date : 2014-12-29
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "ae.h"
#define PORT 12345 
#define SETSIZE 100
#define MAXBUFSIZE 256
typedef struct echoclient {
    int fd;
    int port;
    char ip[20];
    char send_buf[MAXBUFSIZE];
    int send_buf_len;
    char recv_buf[MAXBUFSIZE];
    int recv_buf_len;
    int packet_len;
    ae_event_loop *ev_loop;
} echoclient;
int read_from_client(ae_event_loop *ev_loop, int fd, void *data, int mask);
void reset_echoclient(echoclient *c)
{
    if (!c) return ;
    c->recv_buf[0] = '\0';
    c->recv_buf_len = 0;
    c->send_buf[0] = '\0';
    c->send_buf_len = 0;
    c->packet_len = 0;
}
void free_echoclient(echoclient *c)
{
    if (!c) return ;
    ae_delete_file_event(c->ev_loop, c->fd, AE_READABLE);
    ae_delete_file_event(c->ev_loop, c->fd, AE_WRITABLE);
    close(c->fd);
    free(c);
}
int set_fd_nonblock(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL, flags) == -1)) {
        perror("fcntl GETFL failed\n");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl SETFL failed\n");
        return -1;
    }
    return 0;
}
int create_listen_fd(int backlog)
{
    int fd;
    struct sockaddr_in addr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("create socket failed\n");
        return -1;
    }
    /* 绑定 */ 
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind failed\n");
        close(fd);
        return -1;
    }
    /* 监听 */
    listen(fd, backlog);
    return fd;

}
int readn(echoclient *c, int readlen)
{
    if (!c) return 0;
    int nread;
    nread = read(c->fd, c->recv_buf + c->recv_buf_len, readlen); 
    if (nread == -1) {
        if (errno == EAGAIN || errno == EINTR) {
            return 0;
        }else {
            return -1;
        }
    } else if(nread == 0) {
        return -2;
    }
    c->recv_buf_len += nread;
    return nread;
    
}
int writen(echoclient *c, int writelen)
{
    if (!c) return 0;
    int nwrite = write(c->fd, c->send_buf + c->send_buf_len, writelen);
    if (nwrite == -1) {
        if (errno == EAGAIN || errno == EINTR) {
            return 0;
        }else {
            return -1;
        }
    }
    c->send_buf_len += nwrite;
    return nwrite;
}
int reply_to_client(ae_event_loop *ev_loop, int fd, void *data, int mask)
{
    echoclient *c = (echoclient*)data;
    strncpy(c->send_buf, c->recv_buf, c->packet_len + 1);
    int ret, need_write;
    need_write = c->packet_len + 1 - c->send_buf_len;
    ret = writen(c, need_write);
    if (ret == -1) {
        printf("%s:%d-%d reply to client error\n", c->ip, c->port, c->fd);
        free_echoclient(c);
        return -1;
    }else if (ret == need_write) {
        c->send_buf[c->packet_len+1]  = '\0';
        printf("%s:%d-%d reply client send %s\n", c->ip, c->port, c->fd, c->send_buf);
        if (ae_create_file_event(ev_loop, fd, AE_READABLE, read_from_client, (void*)c) == -1) {
            printf("%s:%d-%d add reply_to_client event failed\n", c->ip, c->port, c->fd);
            free_echoclient(c);
            return -1;
        }
        ae_delete_file_event(ev_loop, fd, AE_WRITABLE);
        reset_echoclient(c);
        
    }

    return 0;

        
}
int read_from_client(ae_event_loop *ev_loop, int fd, void *data, int mask)
{
    echoclient *c = (echoclient*)data;
    int ret, need_read;
    if (c->recv_buf_len == 0) {
        ret = readn(c, 1);//获取内容长度
        if (ret == 0) {
            return 0;
        } else if (ret == -1) {
            printf("%s:%d-%d read from client error\n", c->ip, c->port, c->fd);
            free_echoclient(c);
            return -1;
        }else if (ret == -2) {
            printf("%s:%d-%d client close connection\n", c->ip, c->port, c->fd);
            free_echoclient(c);
            return 0;
        }
        c->packet_len = c->recv_buf[0];
    }
    need_read = c->packet_len - c->recv_buf_len + 1 ;//1字节长度
    ret = readn(c, need_read); 
    if (ret == 0) {
        return 0;
    } else if (ret == -1) {
        printf("%s:%d-%d read from client error\n", c->ip, c->port, c->fd);
        free_echoclient(c);
        return -1;
    } else if (ret == -2) {
        printf("%s:%d-%d client close connection\n", c->ip, c->port, c->fd);
        free_echoclient(c);
        return 0;
    } else if (ret == need_read) {//read finish
        c->recv_buf[c->packet_len+1]  = '\0';
        printf("%s:%d-%d client send %s\n", c->ip, c->port,c->fd, c->recv_buf);
        if (ae_create_file_event(ev_loop, fd, AE_WRITABLE, reply_to_client, (void*)c) == -1) {
            printf("%s:%d-%d add reply_to_client event failed\n", c->ip, c->port, c->fd);
            free_echoclient(c);
            return -1;
        }
        ae_delete_file_event(ev_loop, fd, AE_READABLE);
    }
    return 0;
}
int accept_tcp_handler(ae_event_loop *ev_loop, int listenfd, void *data, int mask)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    while (1) {
        fd = accept(listenfd, (struct sockaddr*)&addr, &len);
        if (fd == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("accept error\n");
                return -1;
            }
        }
        break;
    }
    if (set_fd_nonblock(fd) == -1) {
        return -1;
    }
    echoclient *c = (echoclient*)malloc(sizeof(echoclient));
    if (ae_create_file_event(ev_loop, fd, AE_READABLE, read_from_client, c) == AE_ERR) {
        printf("create read_from_client event failed\n");
        close(fd);
        free(c);
        return -1;
    }
    c->fd = fd;
    c->port = ntohs(addr.sin_port);
    inet_ntop(AF_INET, (void*)&addr.sin_addr, c->ip, sizeof(c->ip));
    c->send_buf[0] = '\0';
    c->recv_buf[0] = '\0';
    c->send_buf_len = 0;
    c->packet_len = 0;
    c->recv_buf_len = 0;
    c->ev_loop = ev_loop;
    printf("%s:%d-%d:client connected to server\n", c->ip, c->port, c->fd);
    return 0;

}
int main(int argc, char **argv)
{
    int listenfd;
    ae_event_loop *ev_loop;

    if ((ev_loop = ae_create_event_loop(SETSIZE)) == NULL) {
        printf("create event loop failed\n");
        return -1;
    }
    if ((listenfd = create_listen_fd(1000)) == -1) {
        ae_delete_event_loop(ev_loop);
        return -1;
    }
    if (ae_create_file_event(ev_loop, listenfd, AE_READABLE, accept_tcp_handler, NULL) == AE_ERR) {
        printf("create listenfd event failed\n");
    }
    ae_main(ev_loop);
    return 0;
}

