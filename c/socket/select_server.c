#include "common.h"
#include <sys/select.h>
#define max(a,b) ((a > b) ? (a) : (b))
int main(int argc, char **argv)
{
    int listen_fd;
    if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        sys_exit("create listen socket failed");
    }

    struct sockaddr_in addr, client_addr;
    socklen_t client_addr_len;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    if (-1 == bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr))) 
        sys_exit("bind failed");
    
    if (-1 == listen(listen_fd, BACKLOG))
        sys_exit("listen failed");
    printf("listening....FD_SETSIZE=%d\n", FD_SETSIZE);
   
    int fds[FD_SETSIZE], sfd, fd;
    int i, nready, nread;
    int maxfd = listen_fd, maxi=-1;
    char buf[2];
    for (i = 0; i < FD_SETSIZE; i++)
        fds[i] = -1;
    fd_set rset, allset;
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);
    
    while (1) {
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(listen_fd, &rset)){/* 客户端连接 */ 
           if (-1 == (sfd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)))
               sys_exit("accept error");
           maxfd = max(maxfd, sfd);
           for (i = 0; i < FD_SETSIZE; i++)
               if (fds[i] == -1) {
                   fds[i] = sfd;
                   break;
               }
           if (i == FD_SETSIZE)
               sys_exit("too many connections");
           FD_SET(sfd, &allset);
           maxi = max(maxi, i);
           if (--nready <= 0) /* 所有发生事件的描述符都处理完成*/
               continue;
        }

        for (i = 0; i <= maxi; i++) {
            if (-1 == (fd = fds[i])) 
                continue;
            if (FD_ISSET(fd, &rset)) {
               if (0 == (nread = read(fd, buf, 2) )) {
                   close(fd);
                   FD_CLR(fd, &allset);
                   fds[i] = -1;
               } else {
                   write(fd, buf,2);
               }
               if (--nready <= 0)
                   break;
            }
        }
    }
    
    return 0;
}

