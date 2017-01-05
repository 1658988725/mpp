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

#include "rtspsrvffmpg.h"


#include "cdr_rec.h"

#include "cdr_rtsp_server.h"


cdr_rec_data_callback g_rec_data_callbak = NULL;
int g_rec_pasueflag = 1;
int g_rec_flag = 0;
unsigned long long g_rec_u64PTS = 0;	
int g_nRecSession = -1;
int g_nRecSessionStatus = 0;

int cdr_rec_rtsp_cb(int nStatus)
{
	g_nRecSessionStatus = nStatus;
	printf("*************************************\r\n");
	printf("%s nStatus:%d\r\n",__FUNCTION__,nStatus);	
	if(g_nRecSessionStatus == 4)
	{
		cdr_rec_start();
		g_nLivePlayFlag = 0;//防止冲突.
	}
	else if(g_nRecSessionStatus == 5)
	{
		cdr_rec_stop();
	}
}


int cdr_rec_rtsp_init(void)
{
	g_rec_flag = 1;
	g_rec_u64PTS = 0;
	
	//创建MP4解析线程.
	//create_parsemp4dir_thread();	

	g_rec_pasueflag = 1;

	//创建MP4读取线程
	cdr_rec_mp4_read();

	//for test
	cdr_rec_registre_data_callbak(rec_stream_callbak);	
	g_nRecSession = cdr_rtsp_create_mediasession("rec.sdp",1920*1080,cdr_rec_rtsp_cb);

	if(g_nRecSession >= 0) return 0;
	
	return -1;
}

//释放所有的资源.
void cdr_rec_realease()
{
	g_rec_flag = 0;
}

void cdr_rec_start(void)
{
	g_rec_pasueflag = 0;
}

void cdr_rec_stop(void)
{
	g_rec_pasueflag = 1;
}


//播放切换到某个时间点.
//Return:0 设置OK
//Return:-1 参数错误.文件不存在.
int cdr_rec_settime(char *pTime)
{
	int nRet = 0;
	cdr_rec_stop();
	printf("%s %s\n",__FUNCTION__,pTime);
	nRet = cdr_mp4ex_seek(pTime);	
	cdr_rec_start();
	return nRet;
}

/*
params:
	pStreamType	:0 video,1 audio.
	pData		:buffer
	nSize		:length of buffer
	freamType		:fame freamType.
return:
单独线程调用，等待执行完毕才会继续执行.
	阻塞函数.	
*/
void cdr_rec_registre_data_callbak(cdr_rec_data_callback callback)
{
	printf("%s \r\n",__FUNCTION__);	
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n");
		return;
	}
	g_rec_data_callbak = callback;		
}



//在存在的mp4文件里面截取某时刻的一段视频.
//pstr的格式:20160727092300_10
//前面代表时刻表.后面代表视频长度.
//截取的视频参数和原先视频参数一样.分辨率.码流.30帧/s
//return:0 截取完成.-1 时间点不在保存的视频里面.
int cdr_get_mp4file(char *pstr)
{
	int nRet = 0;

	printf("%s \r\n",__FUNCTION__);

	int y,m,d,h,M,s,len;
	sscanf(pstr,"%04d%02d%02d%02d%02d%02d_%02d",&y,&m,&d,&h,&M,&s,&len);
	
	struct tm t;
	time_t t_of_day;
	t.tm_year=y-1900;
	t.tm_mon=m-1;
	t.tm_mday=m;
	t.tm_hour=h;
	t.tm_min=M;
	t.tm_sec=s;
	t.tm_isdst=0;
	t_of_day=mktime(&t);
	
	printf("%04d%02d%02d%02d%02d%02d_%02d\r\n",y,m,d,h,M,s,len);
	
	return nRet;
}

//char spspps[40];
/*********************************for test**************************************/
int rec_stream_callbak(int pStreamType,char* pData,unsigned int nSize,int isIFrame)
{
	//printf("%s nSize:%d,isIFrame:%d\n",__FUNCTION__,nSize,isIFrame);
	
	static unsigned long long pts = 0;
	static char exData[64];
	int len = 0;		
	if(isIFrame)
	{	
		memset(exData,0x00,sizeof(exData));
		exData[0] = 0x00;
		exData[1] = 0x00;
		exData[2] = 0x00;
		exData[3] = 0x01;
		memcpy(exData+4,recspsdata,recspslen);
		len = recspslen + 4;
		while(0 != cdr_rtsp_send_frame(g_nRecSession,exData,len,pts))
		{
			usleep(100);
		}
		
		memset(exData,0x00,sizeof(exData));
		exData[0] = 0x00;
		exData[1] = 0x00;
		exData[2] = 0x00;
		exData[3] = 0x01;
		memcpy(exData+4,recppsdata,recppslen);
		len = recppslen + 4;
		while(0 != cdr_rtsp_send_frame(g_nRecSession,exData,len,pts))
		{
			usleep(100);
		}

	}	
	
	while(0 != cdr_rtsp_send_frame(g_nRecSession,pData,nSize,pts));
	{
		usleep(100);		
	}
	pts += 33333;

	usleep(300);
	return 0;
}

