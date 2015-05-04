#include "common.h"
#include <signal.h>
#include <sys/wait.h>

int str_echo(int sfd)
{
    sleep(10);
    char  recv_buf[BUFFERSIZE];
    int len, ret;
    while ((ret = readn(sfd, recv_buf, 1)) > 0) {
       len = recv_buf[0];
       printf("recv len=%d\n", len);
       /*        
       if ((ret = readn(sfd, recv_buf+1, len)) <= 0) 
           break;
       
        
        printf("server finish read\n");
        sleep(20);
        printf("server end sleep\n");
        if ((ret = writen(sfd, recv_buf, len+1)) < 0)
            break;
        printf("server finish write ret=%d\n",ret);
        */
       

    }
    if (-1 == ret)
        sys_exit("read error");
    else if (0 == ret) {
        printf("client closed\n");
    }
    return 0;
}
void sig_chld(int signo)
{
    int stat;
    pid_t pid;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", pid);
    }
    return ;
}
int main(int argc, char **argv)
{
    int sfd, connected_fd;
    struct sockaddr_in addr, client_addr;
    socklen_t client_socket_len;
    if (-1 == (sfd = socket(AF_INET,SOCK_STREAM, 0))) {
        sys_exit("craete socket failed");
    }
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(sfd, (struct sockaddr*)&addr, sizeof(addr))) {
        close(sfd);
        sys_exit("bind failed");
    }

    if (-1 == listen(sfd, /*BACKLOG*/ 1)) {
        close(sfd);
        sys_exit("bind failed");
    }
    if (SIG_ERR == signal(SIGCHLD, sig_chld)) {
        sys_exit("singal error");
    }
    while (1) {
        printf("listening, waiting for client connection\n");
        sleep(120);
        if (-1 == (connected_fd = accept(sfd, (struct sockaddr*)&client_addr, &client_socket_len))) {
            if (EINTR == errno) {
                printf("system call interrupt\n");
                continue;
            } else {
                close(sfd);
                sys_exit("accpet error");
            }
        } 
        /*
        if (0 == fork()) {
            close(sfd);
            str_echo(connected_fd);
            printf("child process exit\n");
            exit(0);
        }
        */
        //str_echo(connected_fd);
        printf("client %s:%d has connected to server!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //close(connected_fd);
    }

    close(sfd);
    return 0;
}

