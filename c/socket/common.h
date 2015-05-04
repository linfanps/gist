#ifndef __COMMON_H
#define __COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#define PORT 12345
#define HOST "127.0.0.1"
//#define HOST "120.132.37.178"
#define BACKLOG 10
#define BUFFERSIZE 1024

ssize_t writen(int fd, void *buf, size_t n);
ssize_t readn(int fd, void *buf, size_t n);
int print_error_msg(const char *msg);
void sys_exit(const char *msg);
#endif
