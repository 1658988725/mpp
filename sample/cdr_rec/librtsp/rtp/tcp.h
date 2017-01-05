
#ifndef _TCP_HEADER_H
#define _TCP_HEADER_H

#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define MSEL                  1000    /* microseconds */

int _selread(int s, int sec);
int _selwrite(int s, int sec);
ssize_t send_tcp_pkg(int socket, void* buff, size_t nbytes, int flags, int*exitflag);
ssize_t recv_tcp_pkg(int socket, void* buff, size_t nbytes, int flags, int*exitflag);

#endif
