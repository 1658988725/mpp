#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "tcp.h"

#define TIMEOUT_SEC 0
#define MAX_WAIT_TIMEOUT_TIMES		4    /* 等待数据可读写超时次数*/

int _selread(int s, int sec) 
{
	if(s < 0)
		return -1;
    fd_set rfd;
    struct timeval tv;
    int rc;

    FD_ZERO(&rfd);
    FD_SET(s, &rfd);
    memset((void *)&tv, 0, sizeof(struct timeval));
    tv.tv_sec = sec;
    do {
    rc = select(s+1, &rfd, NULL, NULL, &tv);
    } while (0);// (rc < 0 && errno == EINTR);
    return rc;

}
  
int _selwrite(int s, int sec)
{
	if(s < 0)
		return -1;
    fd_set wfd;
    struct timeval tv;
    int rc;

    FD_ZERO(&wfd);
    FD_SET(s, &wfd);
    memset((void *)&tv, 0, sizeof(struct timeval));
    tv.tv_usec = 120000;
    tv.tv_sec = sec;
    do {
    	rc = select(s + 1, NULL, &wfd, NULL, &tv);
    } while (0);//(rc < 0 && errno == EINTR);
    return rc;
}

//完成数据发送与接收，出错直接返回send, recv错误值
//以下两函数就要是完成一次无法完成发送与接收的情况
ssize_t send_tcp_pkg(int socket, void* buff, size_t nbytes, int flags, int*exitflag)
{
	//assert(NULL != buff);
	//assert(socket != 0);
	//assert(nbytes != 0);
	unsigned char *pbuff = buff;
	size_t sendbytes = 0;
	size_t leavebytes = nbytes;
	size_t waitcnt = 0;
	ssize_t rv;
	int isready;
	//do{
		//等待SOCKET数据可写
		/*while ((isready=_selwrite(socket, TIMEOUT_SEC))==0)
		{
			usleep(MSEL);
			waitcnt++;
			fprintf(stderr, "select send socket timeout : %d\n", waitcnt);
			if (waitcnt > MAX_WAIT_TIMEOUT_TIMES){
				fprintf(stderr, "send meeting the max timeout : %d\n", waitcnt);
				return -1;
			}
		}*/
		isready=_selwrite(socket, TIMEOUT_SEC);
        if(isready == 0)
        {
			fprintf(stderr, "select return 0..........\n");
			return 0;
        }
		if (isready < 0) 
		{
			fprintf(stderr, "select return -1..........\n");
		    return isready;
		}
		//向SOCKET发送数据
		rv = send(socket, pbuff, leavebytes, flags);
		if (rv < 0){
			fprintf(stderr, "send error........\n");
			return -1;
		} else if (rv == 0){
			return 0;
		}
	/*	leavebytes -= rv;
		sendbytes += rv;
		pbuff += rv;

	}while(sendbytes < nbytes && *exitflag == 0);*/

	return rv;//sendbytes;
}

ssize_t recv_tcp_pkg(int socket, void* buff, size_t nbytes, int flags, int*exitflag)
{
	assert(NULL != buff);
	assert(socket != 0);
	assert(nbytes != 0);
	unsigned char *pbuff = buff;
	size_t recvbytes = 0;
	size_t leavebytes = nbytes;
	size_t waitcnt = 0;
	ssize_t rv;
	int isready;
	do{
		//等待SOCKET可读
		while ((isready=_selread(socket, TIMEOUT_SEC))==0)
		{
			usleep(MSEL);
			waitcnt++;
			fprintf(stderr, "select recv socket timeout : %d\n", waitcnt);
			if (waitcnt > MAX_WAIT_TIMEOUT_TIMES){
				fprintf(stderr, "recv meeting the max timeout : %d\n", waitcnt);
				return -1;
			}
		}
		if (isready < 0) return isready;
		//用阻塞方式读指定长度数据
		rv = recv(socket, pbuff, leavebytes, flags);
		if (rv < 0) {
			return -1;
		} else if (rv == 0){
			return 0;
		}
		leavebytes -= rv;
		recvbytes += rv;
		pbuff += rv;
	}while(recvbytes < nbytes && *exitflag == 0);

	return recvbytes;
}

