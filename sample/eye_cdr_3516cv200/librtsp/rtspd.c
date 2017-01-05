/*******
 * 
 * FILE INFO: software called entrance 
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2013/08/28 15:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2013/09/28 16:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "./rtsp/rtsp.h"
#include "rtspd_api.h"
#include "net/socket.h"
#include "rtp/udp.h"
#include "rtp/tcp.h"
#include "comm/type.h"
#include <netinet/tcp.h>
#include "utils/OSAL_Queue.h"

#include <stdlib.h>
#include <sys/syscall.h>

#define STACK_SIZE 100

int AbortSignal1 = 0;

pthread_cond_t rtspd_cond;
sem_t rtspd_semop_sending_data;
sem_t rtspd_semop;
sem_t h264status;
sem_t rtspd_lock[MAX_CONN];
sem_t rtp_rtcp_lock[MAX_CONN];
pthread_mutex_t rtspd_mutex;

VOID sig_exit(VOID)
{	
	return;// just interrupt the pause()
}

/******************************************************************************/
/*
 *rtsp protocol processing entry function
 
 * ser_ip:  rtsp server socket  listen ip 
 * ser_port:  rtsp server socket  listen port default 554
 */
/******************************************************************************/
S32 proc_rtspd(S32 rtsp_port)
{
	CHAR port[128]="";

	gethostname(rtsp[0]->host_name,32);
	rtsp[0]->rtsp_deport=rtsp_port;
	sprintf(port,"%d",rtsp[0]->rtsp_deport);
	if(create_sercmd_socket(port,SOCK_STREAM)<0)	return -1;

	return 0;
}

/******************************************************************************	
 	函数名称:	getrtspd_version	  
 	函数功能:	流媒体模块初始化	 
 	输入参数:	
 		version: 流媒体版本号
 	输出参数:	无  
 	返回值:		=0	成功	<0	失败	
 	其它说明:    分配内存
******************************************************************************/
S32 getrtspd_version(CHAR *version)
{
	if(!version)
		return -1;
	if(convert_iver_2str(version)<0)
		return -1;
	return 0;
}


/******************************************************************************	
 	函数名称:	rtspd_init	  
 	函数功能:	流媒体模块初始化	 
 	输入参数:	无
 	输出参数:	无  
 	返回值:		=0	成功	<0	失败	
 	其它说明:    分配内存
******************************************************************************/
S32 rtspd_init()
{
	S32 res;
	
	if(init_memory()<0)
	{
		printf("init rtspd memory error\n");
		return -1;
	}
	
	sem_init(&rtspd_semop_sending_data, 0, 1);
	sem_init(&rtspd_semop, 0, 1);
	sem_init(&h264status, 0, 1);
	res = pthread_mutex_init(&rtspd_mutex, NULL);
	if (res != 0) 
	{
		perror("Mutex initialization failed");
		return -1;
	}

	(void)pthread_cond_init(&rtspd_cond, NULL);

	return 0;
}

/******************************************************************************	
 	函数名称:	rtspd_free	  
 	函数功能:	流媒体模块释放	 
 	输入参数:	无  
 	输出参数:	无
 	返回值:		=0	成功	<0	失败	
 	其它说明:    释放内存
******************************************************************************/
S32 rtspd_free()
{
	rtsp_free();
	free_memory();
	pthread_mutex_destroy(&rtspd_mutex);
	pthread_cond_destroy(&rtspd_cond);
	sem_destroy(&rtspd_semop);
	sem_destroy(&rtspd_semop_sending_data);
	sem_destroy(&h264status);
	//sem_destroy(&rtspd_accept_lock);
	return 0;
}

/******************************************************************************	
 	函数名称:	rtspd_staus	  
 	函数功能:	获取流媒体运行状态	 
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		=0	成功	<0	失败	
 	其它说明:    
******************************************************************************/
S32 get_rtspd_status(S32 free_chn)
{
	return rtsp[free_chn]->rtspd_status;
}

/******************************************************************************	
 	函数名称:	rtspd_freechn	  
 	函数功能:	获取流媒体空闲通道号	 
 	输入参数:
 	输出参数:	无
 	返回值:		
 	          >=0  空闲通道号
 	          < 0   获取空闲通道号失败
 	其它说明:    
******************************************************************************/
S32 rtspd_freechn()
{
	S32 i;
	pthread_mutex_lock(&rtspd_mutex);
	DBG("rtspd_freechn pthread_mutex_lock\n");
	pthread_cond_wait(&rtspd_cond,&rtspd_mutex);
	DBG("rtspd_freechn pthread_mutex_unlock\n");
	pthread_mutex_unlock(&rtspd_mutex);
	for(i=0;i<MAX_CONN;i++)
	{
		if(rtsp[i]->conn_status==0)
		{
			DBG("chn:%d, ip : %s\n", i, rtsp[i]->cli_rtsp.cli_host);
		    set_free_conn_status(i,1);
			return i;	
		}
	}
	DBG("%s, %d, ________ERROR_________\n", __func__, __LINE__);
	return -1;	

}

/******************************************************************************	
 	函数名称:	rtsp_init	  
 	函数功能:	rtsp初始化	 
 	输入参数:
 		rtsp_ip:   流媒体监听IP 地址
 		rtsp_port: 流媒体坚挺端口
 	输出参数:	无
 	返回值:		
 				 >=0 初始话rtsp成功
 				 <0  初始话rtsp失败
 									
 	其它说明:    
******************************************************************************/
S32 rtsp_init(S32 rtsp_port)
{
	if(proc_rtspd(rtsp_port)<0){
		return -1;
	}	
	return 0;
}

/******************************************************************************	
 	函数名称:	rtsp_free	  
 	函数功能:	rtsp初始化	 
 	输入参数:
 	输出参数:	无
 	返回值:		
 				 >=0 初始话rtsp成功
 				 <0  初始话rtsp失败
 									
 	其它说明:    建立监听rtsp server 端口
******************************************************************************/
S32 rtsp_free()
{
	if(rtsp[0]->fd.rtspfd > 0)
	{
	    close(rtsp[0]->fd.rtspfd);
		rtsp[0]->fd.rtspfd = -1;
	}
	return 0;
}

S32 rtsp_clifree(int cur_conn_num)
{
	if(rtsp[cur_conn_num]->cli_rtsp.cli_fd > 0)
	{
        close(rtsp[cur_conn_num]->cli_rtsp.cli_fd);
        rtsp[cur_conn_num]->cli_rtsp.cli_fd = -1;
	}
	return 0;
}
    		    

/******************************************************************************	
 	函数名称:	set_framerate	  
 	函数功能:	设置流媒体发送帧率
 	输入参数:
 		f_rate:      帧率
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 发送数据失败
 				 <0  发送数据成功
 									
 	其它说明:    
******************************************************************************/
S32 set_framerate(S32 f_rate,S32 free_chn)
{
	switch(f_rate){
		case 25:
			rtsp[free_chn]->cmd_port.frame_rate_step=3600;
			rtsp[free_chn]->cmd_port.audio_step = 180;
			break;
			
		case 30:
			rtsp[free_chn]->cmd_port.frame_rate_step=3000;
			rtsp[free_chn]->cmd_port.audio_step = 180;
			break;
			
		default: 
			printf("Default frame rate 25 fpbs\n");
			rtsp[free_chn]->cmd_port.frame_rate_step=3600;
			rtsp[free_chn]->cmd_port.audio_step = 180;
			break;
	}
	return 0;
}


/******************************************************************************	
 	函数名称:	rtsp_proc	  
 	函数功能:	rtsp协议命令行解析处理
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 命令解析模块调用成功
 				 <0  命令解析模块调用失败
 									
 	其它说明:    
******************************************************************************/ 
S32 rtsp_proc(S32 free_chn)
{
	if(pthread_create(&rtsp[free_chn]->pth.rtsp_proc_thread, NULL, vd_rtsp_proc,(VOID *)free_chn) < 0)
	{				
		printf("pthread_create vd_rtsp_proc thread error\n");
		return -1;
	}
	DBG("rtsp_proc func exit\n");
	return 0;
}

void *rtp_rtcp_init(void *arg)
{
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	S32 free_chn=0;
	pthread_detach(pthread_self());

	free_chn=(S32)arg;
    DBG("rtp_rtcp_init begin\n");

	if(rtp_init(free_chn) == 0){
	    rtcp_init(free_chn);
	}
	printf("chn: %d, ip: %s, thread rtp_rtcp_init exit\n", free_chn, rtsp[free_chn]->cli_rtsp.cli_host);
	pthread_exit(NULL);
	return NULL;

}

S32 rtp_rtcp_proc(S32 free_chn)
{

	if(pthread_create(&rtsp[free_chn]->pth.rtp_rtcp_init_thread, NULL, rtp_rtcp_init,(VOID *)free_chn) < 0){				
		printf("pthread_create rtp_rtcp_proc thread error\n");
		return -1;
	}
	DBG("rtp_rtcp_proc func exit\n");
	return 0;
}



/******************************************************************************	
 	函数名称:	rtp_init	  
 	函数功能:	rtp协议初始化
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 rtp协议初始化成功
 				 <0  rtp协议初始化失败
 									
 	其它说明:    
******************************************************************************/
S32 rtp_init(S32 free_chn)
{	
	S32 ret;
	printf("****************chn: %d, ip: %s, wait rtsp message complete****************\n",free_chn,rtsp[free_chn]->cli_rtsp.cli_host);
	sem_wait(&rtspd_lock[free_chn]);
	ret = get_rtspd_status(free_chn);
	if(ret != 0x08){
		printf("Waring: rtsp chn[%d], ip: %s, status is 0x%x\n",free_chn,rtsp[free_chn]->cli_rtsp.cli_host, ret);
		return -1;
	}
	printf("****************chn: %d, ip: %s, rtsp play recv****************\n",free_chn,rtsp[free_chn]->cli_rtsp.cli_host);

    if(rtsp[free_chn]->rtp_tcp_mode == RTP_UDP )
    {
     	if(ret = create_vrtp_socket(rtsp[free_chn]->cli_rtsp.cli_host,
    		rtsp[free_chn]->cmd_port.rtp_udp_track1_port,rtsp[free_chn]->cmd_port.rtp_udp_track2_port,
    		SOCK_DGRAM,
    		free_chn))
    	{
			sem_wait(&rtspd_semop_sending_data);
    		rtsp_tcp_para_clear(free_chn);
			sem_post(&rtspd_semop_sending_data);
    	    printf("Create rtp socket error\n");
    		return -1;
    	}	
    } 
    else
    {
		
		int nodelay = 1;
    	setsockopt(rtsp[free_chn]->fd.video_rtp_fd,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof(nodelay));
    	int KeepAlive1 = 0;
    	setsockopt(rtsp[free_chn]->fd.video_rtp_fd,SOL_TCP,TCP_CORK,(char*)&KeepAlive1,sizeof(KeepAlive1));
    	
        rtsp[free_chn]->fd.video_rtp_fd = rtsp[free_chn]->cli_rtsp.cli_fd;
        rtsp[free_chn]->fd.audio_rtp_fd = rtsp[free_chn]->cli_rtsp.cli_fd;  
    }

	return 0;
}

/******************************************************************************	
 	函数名称:	send_file	  
 	函数功能:	通过文件发送数据的函数接口
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 发送数据失败
 				 <0  发送数据成功
 									
 	其它说明:    
******************************************************************************/
S32 send_file(S32 free_chn)
{
	if (pthread_create(&rtsp[free_chn]->pth.rtp_vthread,NULL,vd_rtp_func,(VOID *)free_chn)<0){
		printf("pthread_create rtcp error:\n");
		return -1;
	}
	return 0;
}


/******************************************************************************	
 	函数名称:	rtp_free	  
 	函数功能:	rtp协议注销
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 rtp协议初始化成功
 				 <0  rtp协议初始化失败
 									
 	其它说明:    
******************************************************************************/
S32 rtp_free(S32 free_chn)
{
	if(rtsp[free_chn]->fd.video_rtp_fd > 0)
	{
	    close(rtsp[free_chn]->fd.video_rtp_fd);
		rtsp[free_chn]->fd.video_rtp_fd = -1;
	}
	if(rtsp[free_chn]->fd.audio_rtp_fd > 0)
	{
	    close(rtsp[free_chn]->fd.audio_rtp_fd);
		rtsp[free_chn]->fd.audio_rtp_fd = -1;
	}
	return 0;
}

/******************************************************************************	
 	函数名称:	rtcp_init	  
 	函数功能:	rtcp协议初始化
 	输入参数:
 		freechn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 rtcp协议初始化成功
 				 <0  rtcp协议初始化失败
 									
 	其它说明:    
******************************************************************************/
S32 rtcp_init(S32 free_chn)
{
	int ret = -1;
	
	sem_wait(&rtspd_semop_sending_data);
	rtsp[free_chn]->is_sending_data = 1;
	sem_post(&rtspd_semop_sending_data);

    /*基于UDP的RTCP需要创建新的socket LiuShujie*/
    if(rtsp[free_chn]->rtp_tcp_mode == RTP_UDP)
    {
    	if(ret = create_vrtcp_socket(rtsp[free_chn]->cli_rtsp.cli_host,
    		rtsp[free_chn]->cmd_port.rtcp_udp_track1_port,rtsp[free_chn]->cmd_port.rtcp_udp_track2_port,
    		SOCK_DGRAM,
    		free_chn))
    	{
    		if(ret == -1)
    		    rtcp_free(free_chn);
    	    printf("Create rtcp socket error\n");
    		return -1;
    	}
        /* //20140606 上电启动OVNIF接入海康NVR偶现主进程关闭，定位断错误在此线程
        if (pthread_create(&rtsp[free_chn]->pth.rtcp_vthread, NULL, vd_rtcp_recv,(VOID *)free_chn) < 0){            
            printf("pthread_create rtcp error:\n");
            return -1;
        }
         */  
        if (pthread_create(&rtsp[free_chn]->pth.rtcp_vthread1, NULL, vd_rtcp_send,(VOID *)free_chn) < 0){           
            printf("pthread_create rtcp error:\n");
            return -1;
        }


    }
    else
    {
        rtsp[free_chn]->fd.video_rtcp_fd = rtsp[free_chn]->cli_rtsp.cli_fd;
        rtsp[free_chn]->fd.audio_rtcp_fd = rtsp[free_chn]->cli_rtsp.cli_fd;
	
        if (pthread_create(&rtsp[free_chn]->pth.rtcp_vthread1, NULL, vd_rtcp_tcp_send,(VOID *)free_chn) < 0){           
            printf("pthread_create rtcp error:\n");
            return -1;
        }
    }   
	return 0;

}

/******************************************************************************	
 	函数名称:	rtspd_vtype	  
 	函数功能:	rtsp数据流访问方式
 	输入参数:
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
				0	H264文件方式访问
				1	PS文件访问
				2	实时流方式访问
        <0 失败
					
 	其它说明:    
******************************************************************************/
S32 rtspd_vtype(S32 free_chn)
{
	return rtsp[free_chn]->vist_type;
}


/******************************************************************************	
 	函数名称:	rtcp_free	  
 	函数功能:	rtcp协议注销
 	输入参数:
 		freechn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 rtp协议初始化成功
 				 <0  rtp协议初始化失败
 									
 	其它说明:    
******************************************************************************/
S32 rtcp_free(S32 free_chn){
    if(rtsp[free_chn]->fd.video_rtcp_fd > 0)
    {
	    close(rtsp[free_chn]->fd.video_rtcp_fd);
		rtsp[free_chn]->fd.video_rtcp_fd = -1;
    }
	if(rtsp[free_chn]->fd.audio_rtcp_fd > 0)
	{
	    close(rtsp[free_chn]->fd.audio_rtcp_fd);
		rtsp[free_chn]->fd.audio_rtcp_fd = -1;
	}
    return 0;
}


/******************************************************************************	
 	函数名称:	rtp_svpactet	  
 	函数功能:	发送视频数据函数接口
 	输入参数:
 		buff:      一帧视频数据buf
 		framesize: 一帧数据长度
 		free_chn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 >=0 发送数据失败
 				 <0  发送数据成功
 									
 	其它说明:    
******************************************************************************/
S32 rtp_svpactet(U8 *buff ,S32 framesize,S32 free_chn)
{
	if(!buff)
		return -1;

    if(rtsp[free_chn]->rtp_tcp_mode==RTP_TCP)
    {
        //sem_wait(&rtp_rtcp_lock[free_chn]);
        if(build_rtp_tcp_nalu(buff, framesize, free_chn,0,0,0,0)<0){
            printf("send video packet error\n");
            return -1;
        }
        //sem_post(&rtp_rtcp_lock[free_chn]); 
    }
    else
    {
	    if(build_rtp_nalu(buff, framesize, free_chn,0,0,0,0)<0){
            printf("send video packet error\n");
            return -1;
        }

    } 
	return 0;
}



/******************************************************************************	
 	函数名称:	rtsp_freeall	  
 	函数功能:	释放rtp rtcp数据通道
 	输入参数:

 	输出参数:	无
 	返回值:		
 				 >=0 发送数据失败
 				 <0  发送数据成功
 									
 	其它说明:    
******************************************************************************/
S32 rtsp_freeall()
{
	S32 i;

	for(i=0;i<MAX_CONN;i++){
		if(rtsp[i]->is_runing){
			rtp_free(i);
			rtcp_free(i);
		}
	}
	return 0;
}

/******************************************************************************	
 	函数名称:	rtspd_chn_quit	  
 	函数功能:	rtspd 数据通道断开状态判断
 	输入参数:
 		freechn: 空闲通道号 
 	输出参数:	无
 	返回值:		
 				 =0  数据通道已断开
 				 =1 数据通道正在使用
 									
 	其它说明:    
******************************************************************************/
S32 rtspd_chn_quit(S32 free_chn)
{
	return rtsp[free_chn]->is_runing;
}

pthread_t rtspd_vthread;

int rtsp_callback()
{
	S32 free_chn = 0;
	while(1)
	{
		printf("rtsp_callback func wait.............\n");
		free_chn=rtspd_freechn();
		if(rtsp_proc(free_chn) < 0) continue;
		if(rtp_rtcp_proc(free_chn) < 0) continue;
     }
	return 0;
}

VOID *vd_rtspd_func(VOID *arg)
{
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	pthread_detach(pthread_self());
	rtsp_callback();
	DBG("-----------------------thread vd_rtspd_func exit---------------------\n");
	pthread_exit(NULL);
	return NULL;
}

int encoder_func(void * ptr, int size, OMX_S64 timeStamp)
{
	int ret = 0;
#if 0	
	static int index = 0;
	
	if(OSAL_GetElemNum(&mQueueBuffer) >= NB_BUFFER){
		printf("*********no free buffer**********\n");
        return -1;

	}
	index %= NB_BUFFER;
	memcpy((unsigned char *)bufPtr[index], (unsigned char *)ptr, size);
	mBuf[index].data = bufPtr[index];
	mBuf[index].size = size;
	mBuf[index].time = timeStamp;
	//printf("queue buffer...index %d, size %d\n", index, size);
    ret = OSAL_Queue(&mQueueBuffer, &mBuf[index]);
	if (ret != 0)
	{
		//printf("preview queue full...\n");
		return -1;
	}
    index ++;
#endif	
	return ret;
}

void queue_free(void)
{
	int i=0,j=0;
	for(j = 0;j<MAX_MEDIO_SESSION;j++)
	{
		struct rtsp_media_session *pMs = &g_media_session[j];
		
		if(strlen(pMs->media_session_name) <= 0) continue;
		
		OSAL_QueueTerminate(&pMs->mQueueBuffer);
		for(i = 0; i< NB_BUFFER; i++)
		{
			if(pMs->bufPtr[i] != NULL)
			{
				free(pMs->bufPtr[i]);
				pMs->bufPtr[i] = NULL;
			}
		}
	}

}


static void signalHandler(void)
{
	fprintf(stderr,"RTSP Server caught SIGTERM: shutting down\n");
	AbortSignal1 = 1;
	rtspd_free();
	queue_free();
}

int cdr_rtsp_init(int nPort)
{
	signal(SIGINT, (VOID*)signalHandler);

	//1. create rtsp listen socket;
	int ret = rtspd_init();    
	if(ret >= 0)
     {
		if(rtsp_init(nPort) < 0)	goto PROC_EXIT;
        
		if (pthread_create(&rtspd_vthread, NULL, vd_rtspd_func,NULL) < 0)
		{
			printf("pthread_create vd_rtspd_func error:\n");
			goto PROC_EXIT;
		}
	}
	//cdr_system("echo -e \"9.Start rtsp\r\n\" >> /dev/ttyAMA2");	//tell com2 live start.
	return 0;
	
PROC_EXIT:
	rtspd_free();
	queue_free();
}


int cdr_rtsp_release()
{
    rtspd_free();
    queue_free();
}


extern VOID * rtp_send_form_stream_func(VOID *arg);

//create mediasession
//nMaxFrameLen 1920*1080 or 640*480.
int cdr_rtsp_create_mediasession(char *pMediaName,int nMaxFramelen,_rtsp_mediasession_cb fcb)
{
	static int mediasessioncount = 0;
	int nRet = -1;
	int i = 0;
    
	if(mediasessioncount >= MAX_MEDIO_SESSION)
	{
		printf("max media session support\r\n");
		return -1;
	}
	if(pMediaName == NULL)	return -1;
    
	struct rtsp_media_session *pMs = &g_media_session[mediasessioncount];
	
	// init  buffer queue
	OSAL_QueueCreate(&pMs->mQueueBuffer, NB_BUFFER);

    for(i=0; i< NB_BUFFER; i++)
	{    
    	    pMs->bufPtr[i] = (void *)malloc(nMaxFramelen);
		if(pMs->bufPtr[i] == NULL)
		{
            printf("[%s %d] num %d malloc fail \n",__FUNCTION__,__LINE__,i);
		   goto PROC_EXIT;
		}
	}

	strcpy(pMs->media_session_name,pMediaName);
	pMs->conn_index = -1;
	nRet = mediasessioncount;
	pMs->mediasession_fn = fcb;
		
  	//rtsp cmd thread
  	if (pthread_create(&pMs->rtsp_send_thread, NULL, rtp_send_form_stream_func,mediasessioncount) < 0)
	{			  
		printf("pthread_create rtp_send_form_stream_func thread error\n");
	  	goto PROC_EXIT;
  	}	
	mediasessioncount++;

	return nRet;
PROC_EXIT:
	queue_free();
	return -1;

}

//send frame.
int cdr_rtsp_send_frame(int nSession,void * ptr, int size, OMX_S64 timeStamp)
{
    static int index = 0;
    int ret = 0;

    if(nSession < 0 || nSession > MAX_MEDIO_SESSION)
    {
        	printf("[%s,%d] session %d invalid.\n",__FUNCTION__,__LINE__,nSession);
        	return -1;
    }

    struct rtsp_media_session *pMs = &g_media_session[nSession];
    if(OSAL_GetElemNum(&pMs->mQueueBuffer) >= NB_BUFFER)
    {
        	printf("[%s,%d]no free buffer\n",__FILE__,__LINE__);
        	return -1;
    }

    index %= NB_BUFFER;
    memcpy((unsigned char *)pMs->bufPtr[index], (unsigned char *)ptr, size);
    pMs->mBuf[index].data = pMs->bufPtr[index];
    pMs->mBuf[index].size = size;
    pMs->mBuf[index].time = timeStamp;
    	
    ret = OSAL_Queue(&pMs->mQueueBuffer, &pMs->mBuf[index]);
    if (ret != 0) 	return -1;

    index ++;
    
    return 0;
}

int cdr_reset_mediassion(int nSession)
{
    
	struct rtsp_media_session *pMs = &g_media_session[nSession];
	pMs->nend_iframe = 1;	

     return 0;
}


