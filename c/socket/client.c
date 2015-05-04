#include "common.h"
#include <signal.h>
void sig_pipe_handler(int sig_no)
{
    printf("catch SIGPIPE signal");
}

int str_cli(int sfd)
{
    char send_buf[BUFFERSIZE], recv_buf[BUFFERSIZE];
    int i, n, len;
    while (NULL != fgets(send_buf, BUFFERSIZE, stdin)) {
        len = strlen(send_buf);
        for (i = len - 1; i >= 1; i--) {
            send_buf[i] = send_buf[i-1];
        }
        send_buf[0] = len - 1;
        n = writen(sfd, send_buf, len);
        if ( -1 == n) 
            sys_exit("write error");
        else if (0 == n)
            sys_exit("server closed write return 0");
        printf("client finish write len=%d\n", n);
        /*
          test SIGPIPE

        signal(SIGPIPE, sig_pipe_handler);        
        n = writen(sfd, send_buf, strlen(send_buf));
        */
        n = readn(sfd, recv_buf, strlen(send_buf));
        if (-1 == n) {
            sys_exit("read error");
        } else if (0 == n) {
            printf("server closed\n");
            break;
        }
        recv_buf[strlen(send_buf)] = '\0';
        printf("client finish read\n");
        fputs(recv_buf, stdout);
    }
    
    return 0;
}
int test_nagle_algorithm(int fd)
{
    /* 发100个包，每个包1字节 用tcpdump观察是否启用了nagel算法*/
    int i, len;
    char onek[1024];
    for (i = 0; i < 1024; i++) {
        onek[i] = '1';
    }
    for (i = 0; i < 50; i++) {
        if ((len = write(fd, onek, 1024)) < 0) {
            printf("write error msg:%s\n", strerror(errno));
        } else if (len == 0) {
            printf("server closed\n");
            break;
        }
        printf("sending %d\n", i);
    }
    return 0;
}
int main(int argc, char **argv)
{
    int sfd;
    struct sockaddr_in addr;
    int ret;
    int send_buf_len;
    socklen_t len = sizeof(int);

    if (-1 == (sfd = socket(AF_INET, SOCK_STREAM, 0))) {
        sys_exit("craete socket failed");
    }
    
    getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (char*)&send_buf_len, &len);
    printf("send_buf_len=%d\n", send_buf_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (-1 == inet_pton(AF_INET, HOST, &addr.sin_addr)) {
        sys_exit("host wrong format");
    }

    /* 禁用nagle算法 */
    int var = atoi(argv[1]);
    setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &var, sizeof(var));
    if (-1 == connect(sfd, (struct sockaddr*)&addr, sizeof(addr))) {
        close(sfd);
        sys_exit("connect to server failed");
    }
    printf("connected to server\n");
    sleep(1000);
    //str_cli(sfd);
    test_nagle_algorithm(sfd);
    close(sfd);
    return 0;
}

