CC=gcc
CFLAGS=-g -Wall

client: client.c common.o
	$(CC) $(CFLAGS) client.c common.o -o client

server: server.o common.o 
	$(CC) $(CFLAGS) server.c common.o -o server 

select_server: select_server.o common.o 
	$(CC) $(CFLAGS) select_server.c common.o -o select_server 

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

.PHONY: all
all: client server select_server
