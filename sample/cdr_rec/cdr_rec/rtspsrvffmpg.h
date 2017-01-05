#ifndef __RTSPSRVFFMPEG_H_
#define __RTSPSRVFFMPEG_H_

//#include <libavutil/timestamp.h>
//#include <libavformat/avformat.h>


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
//#include <syswait.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <pthread.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <netinet/if_ether.h>
#include <net/if.h>

#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <dirent.h>



typedef struct _RTPSendBuf
{
	unsigned int rtpSendBufLen;
	unsigned int oCurretFrame;
	int rtpIFrame;
	unsigned long rtpSendDelay;
	char rtpSendBuf[2000000];
}RTPSendBuf;


extern RTPSendBuf g_oRTPSendBuf;


//int pack_mp4_rtsp(void);
int rec_stream_callbak(int pStreamType,char* pData,unsigned int nSize,int isIFrame);



void rec_pool_init(void);
void rec_pool_reset(void);
#endif
