#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void err_sys(const char *msg)
{
    printf("msg:%s, errno:%d, errmsg:%s\n", msg, errno, strerror(errno));
}
static int set_blocking(int fd, int blocking)
{
    int flags;
    if (-1 == (flags = fcntl(fd,F_GETFL))) 
        return -1;
    
    if (blocking)
        flags &= ~O_NONBLOCK;
    else 
        flags |= O_NONBLOCK;

    if (-1 == fcntl(fd, F_SETFL, flags))
        return -1;
    return 0;
}
static int connect_nonblock(const char *ip, int port ,int timeout)
{
    int sfd;
    if (-1 == (sfd = socket(AF_INET, SOCK_STREAM, 0))) {
        err_sys("create socket failed");
        return -1;
    }
    
    if (-1 == set_blocking(sfd, 0)) {
        err_sys("set sfd nonblock failed");
        close(sfd);
        return -1;
    }
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ( 0 ==inet_aton(ip, &(addr.sin_addr))) {
        err_sys("address is invalid");
        close(sfd);
        return -1;
    }

    if (-1 == connect(sfd, (struct sockaddr *)&addr, sizeof(addr))) {
        if (errno != EINPROGRESS) {
            err_sys("can not conenct to server");
            close(sfd);
            return -1;
        }
        printf("EINPROGERESS.....\n");
        struct timeval tval;
        tval.tv_sec = 0;
        tval.tv_usec = timeout;

        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(sfd, &wset);
       
       if (0 == select(sfd+1, NULL, &wset, NULL, &tval)) {
           errno = ETIMEDOUT;
           err_sys("non-block select connect timeout");
           close(sfd);
           return -1;
       }
       
       if (FD_ISSET(sfd, &wset)) {
           int err,code;
           int errlen = sizeof(err);
           code = getsockopt(sfd, SOL_SOCKET, SO_ERROR, &err, &errlen);
          /* 如果发生错误，Solaris实现的getsockopt返回-1， 
         * 把pending error设置给errno. Berkeley实现的 
         * getsockopt返回0, pending error返回给error.  
         * 我们需要处理这两种情况 */ 
           if (code < 0 || err) {
                if (err) 
                    errno = err;
                err_sys("getsockopt check error");
                close(sfd);
                return -1; 
           }
        
       } else {
            err_sys("select error");
            close(sfd);
            return -1;
       }

    }
    if (-1 == set_blocking(sfd, 1)) {
        err_sys("resume sfd block failed");
        close(sfd);
        return -1;
    }
    printf("connected....\n");
    return 0;

}
int main(int argc, char **argv)
{
    connect_nonblock("180.149.134.142", 80, 10000);
    while (1) {
        sleep(1000);
    }
    return 0;
}

