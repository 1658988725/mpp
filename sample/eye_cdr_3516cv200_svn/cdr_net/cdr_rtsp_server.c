
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>

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
#include "cdr_rtsp_server.h"
#include "cdr_mpp.h"

int g_nLiveSession = -1;
int g_nLiveSessionStatus = 0;
int g_nLivePlayFlag = 0;

//g_nLiveSessionStatus == 4 is play cmd.
//make sure first frame is iFrame.
//add by lilin 20160816.
extern int g_nLiveSessionStatus;

int live_stream_callbak(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame)
{
	static int iFrameFlag = 0;
    
	if(g_nLivePlayFlag == 1)
	{
        if(iFrameFlag == 1 && vaFrame.isIframe != 1) 	return 0;
        
		cdr_rtsp_send_frame(g_nLiveSession,pData,nSize,vaFrame.u64PTS);	
		iFrameFlag = 0;
	}else{
		iFrameFlag = 1;
	}
	
	return 0;
}


int cdr_live_rtsp_cb(int nStatus)
{
	g_nLiveSessionStatus = nStatus;
    
	if(g_nLiveSessionStatus == 4){
		cdr_rec_stop();//这个地方需要关闭rec，复制重复冲突.最对只支持一路流播放.
		g_nLivePlayFlag = 1;
	}

	if(g_nLiveSessionStatus == 5){
		g_nLivePlayFlag = 0;
	}
	
	printf("%s nStatus:%d\r\n",__FUNCTION__,nStatus);	
}


int cdr_live_rtsp_init(void)
{ 
	cdr_stream_setevent_callbak(CDR_LIVE_TYPE,&live_stream_callbak);
	g_nLiveSession = cdr_rtsp_create_mediasession("live.sdp",640*480,cdr_live_rtsp_cb);	
	if(g_nLiveSession >= 0) return 0;
	return -1;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
