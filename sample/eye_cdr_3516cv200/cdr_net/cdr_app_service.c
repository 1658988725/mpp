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
#include <malloc.h>
#include <dirent.h>

#include "cdr_app_service.h"
#include "cdr_protocol.h"
#include "cdr_writemov.h"
#include "minxml/mxml.h"

#include "cdr_bma250e.h"
#include "cdr_device_discover.h"

#include <sys/vfs.h>
#include <assert.h>
#include <net/if.h> 

#include "cdr_comm.h"
#include "cdr_config.h"
#include "cdr_bt.h"

#include "cdr_device.h"
#include "cdr_XmlLog.h"
#include "cdr_audio.h"
#include "cdr_mpp.h"
#include "cdr_mp4_api.h"

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef TRUE
	#define TRUE  1
#endif

typedef enum
{
	APPCLIENT_IDLE = 0,
	APPCLIENT_CONNECTED = 1,
	APPCLIENT_SENDING = 2,
}APPCLIENT_STATUS;

typedef struct
{
	int index;
	int socket;
	int status;
    int iOverTimeCnt;
    
    char loginflag;
	char IP[20];
	char token[32];
	CDR_PROTOCOL proRecvBuffer; //tcp recv buffer
	CDR_PROTOCOL proSendBuffer; //tcp send buffer
	    
     pthread_t ClientMsgThreadID;

}APP_CLIENT;

//需要主动返回给app的信息.
typedef struct _sAckCmdList
{
	struct _sAckCmdList *pNext;
	unsigned char  nCmdId;
	char msgBody[100];
}AckCmdList;

AckCmdList *g_AckCmdlist = NULL;


#define APP_SERVER_TCP_PORT 12330
#define MAX_APP_CLIENT 2
APP_CLIENT g_appclients[MAX_APP_CLIENT];

#define CDR_VERSION 0x0100

#define CDR_UPLOAD_FILE_NAME "/mnt/mmc/tmp/update.zip"
#define USERNAMEANDPASSWD "username:EKAKA1&passwd:1234567"
#define LOGIN_ACK_BODY "cdrSN:1234567890&cdrSoftwareVersion:110&cdrTime:20140604144100"

#define STR_INDEXPIC "index_list.xml"
#define STR_MP4LIST "mp4_list.xml"
#define STR_JPGLIST "jpg_list.xml"
#define STR_SDINFO "cdr_sdinfo.xml"
#define STR_SYSTEMINFO "cdr_ver.xml"
#define STR_SYSCFG "cdr_syscfg.xml"
#define STR_GPSCFG "cdr_gps_list.xml"


#define XMLFILE_JPG_LIST "/mnt/mmc/tmp/jpg_list.xml"
#define XMLFILE_MP4_LIST "/mnt/mmc/tmp/mp4_list.xml"
#define XMLFILE_SD_INFO  "/mnt/mmc/tmp/cdr_sdinfo.xml"
#define XMLFILE_INDEX_LIST "/mnt/mmc/tmp/index_list.xml"
#define XMLFILE_GPS_LIST "/mnt/mmc/tmp/cdr_gps_list.xml"

#define OVER_TIME_SEC  60+10 //60+10//40

void parse_app_cmd(char* pData,int len,int clientIndex);
char* get_token_string(int tokenlen);
int update_mp4_list_xml(void);
int update_jpg_list_xml(void);
static unsigned long cdr_get_file_size(const char *path);

extern char g_cdr_device_app_stop_flag;


static unsigned char *pucSmartUnitID = "13600020001";//test

#if 0
static char const *dateHeader() 
{
	static char buf[200];
	time_t tt = time(NULL);
	strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
	return buf;

} 
#endif

//拍照关联视频处理函数.
//flag:1 关联视频，只生成图片文件eg:20161209095800_10.jpg
//flag:2 生成图片文件，同时会生成mp4文件.
int cdr_AssociatedVideo(int flag,int len)
{	
	if(flag <=0 || len <= 0)
		return 0;
	
	cdr_mp4_setlen(g_mp4handle,len);
	if(cdr_mp4_getextrainfo(g_mp4handle) == 0)
	{
		char fullJpgName[256];
		time_t curTime;
		struct tm *ptm = NULL;
		memset(fullJpgName,0x00,256);
		time(&curTime);
		ptm = gmtime(&curTime);

		sprintf(fullJpgName, "/mnt/mmc/PHOTO/%04d%02d%02d%02d%02d%02d",ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		fullJpgName[strlen(fullJpgName)] = '\0';
		//printf("fullName:%s\n",fullJpgName);
		cdr_save_jpg(gsqcif_stPara.s32VencChn,fullJpgName);
		cdr_mp4_setextrainfo(g_mp4handle,fullJpgName);
		if(flag == 2)
			cdr_mp4_outputmp4flag(g_mp4handle,1);
	}
	return 0;
}

static char* GetLocalIP(int sock)
{
	struct ifreq ifreq;
	struct sockaddr_in *sin;
	char * LocalIP = malloc(20);
	strcpy(ifreq.ifr_name,"ra0");
	if (!(ioctl (sock, SIOCGIFADDR,&ifreq)))
	{ 
		sin = (struct sockaddr_in *)&ifreq.ifr_addr;
		sin->sin_family = AF_INET;
		strcpy(LocalIP,inet_ntoa(sin->sin_addr)); 
		//inet_ntop(AF_INET, &sin->sin_addr,LocalIP, 16);
	} 
	return LocalIP;
}

void  AppClientMsg(void* pParam)
{
	int nRes;
	char pRecvBuf[1024] = {0};

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);           //允许退出线程   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,   NULL);    //设置立即取消 

	pthread_detach(pthread_self());
    
	APP_CLIENT * pClient = (APP_CLIENT*)pParam;

	memset(pRecvBuf,0,sizeof(pRecvBuf));
	printf("app----Create Client %s\n",pClient->IP);
	while(pClient->status != APPCLIENT_IDLE)
	{         
		memset(pRecvBuf,0x00,sizeof(pRecvBuf));
		nRes = recv(pClient->socket, pRecvBuf, 1024,0);
		if(nRes < 1)
		{
			printf("app recv Error--- %d\n",nRes);
			g_appclients[pClient->index].status = APPCLIENT_IDLE;
			close(pClient->socket);
			g_appclients[pClient->index].socket = -1;
            pClient->iOverTimeCnt = 0;
			break;
		}
		parse_app_cmd(pRecvBuf,nRes,pClient->index);
	}
	printf("app:-----Exit Client %s\n",pClient->IP);
	return NULL;
}


void AppClientAlive()
{
   int i = 0;
   pthread_detach(pthread_self());    
   while(1)
   {
      sleep(1);      
      for(i=0;i<MAX_APP_CLIENT;i++)
      {   
        if(g_appclients[i].status != APPCLIENT_IDLE)
        {
            g_appclients[i].iOverTimeCnt++;
            if(g_appclients[i].iOverTimeCnt>= OVER_TIME_SEC)
            {
                printf("[%s %d]g_appclients[%d] link over timer\n",__FUNCTION__,__LINE__,i);
                g_appclients[i].status = APPCLIENT_IDLE;
                close(g_appclients[i].socket);
				g_appclients[i].socket = -1;
				
                pthread_cancel(g_appclients[i].ClientMsgThreadID);
                g_appclients[i].iOverTimeCnt = 0;
            }
        }
      }
   }
}

//添加一条ack 消息到队列..
int cdr_add_ackmsg(unsigned char nCmdID,char *pMsg)
{
	//printf("%s CMDID:%d,msgbody:%s\n",__FUNCTION__,nCmdID,pMsg);
	
	AckCmdList *p,*pList = NULL;
	p = (AckCmdList*)malloc(sizeof(AckCmdList));
	if(p == NULL) return -1;
	
	p->pNext = NULL;
	memset(p->msgBody,0x00,100);
	memcpy(p->msgBody,pMsg,strlen(pMsg));
	p->nCmdId = nCmdID;
	
	if(g_AckCmdlist == NULL)
	{
		g_AckCmdlist = p;
	}
	else
	{
		pList = g_AckCmdlist;
		while(pList->pNext) pList = pList->pNext;
		
		pList->pNext = p;	
	}
	return 0;
}

//ack msg to clint.
//因为用的阻塞socket.有些场合需要反馈消息给所有连接的app.socket异常断开后.就会导致线程阻塞.
//所以单独放到一个ack线程里面来处理这些ack消息.
void AckClientPro(void)
{
	int i = 0;
	AckCmdList *p = NULL;
	pthread_detach(pthread_self());    
	while(1)
	{
		usleep(10000);      
		if(!g_AckCmdlist) continue;

		p = g_AckCmdlist;
		g_AckCmdlist = g_AckCmdlist->pNext;
		
		for(i=0;i<MAX_APP_CLIENT;i++)
		{  
			
			//if(g_appclients[i].loginflag == 0 || g_appclients[i].socket <= 0 || g_appclients[i].status == APPCLIENT_IDLE)
			if(g_appclients[i].loginflag == 0 || g_appclients[i].status == APPCLIENT_IDLE)
			{				
				continue;
			}			
			pack_ack_packet(i,p->nCmdId,p->msgBody,strlen(p->msgBody));
			send_ack_data(i);			
		}
		free(p);
		p = NULL;
	}
}


void  AppServerListen(void*pParam)
{
	int s32Socket;
	struct sockaddr_in servaddr;
	int s32CSocket;
    int s32Rtn;
    int s32Socket_opt_value = 1;
	int nAddrLen;
	struct sockaddr_in addrAccept;
	int i = 0;		  
	int bAdd=FALSE;
	char *pToken = NULL;
	
	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(APP_SERVER_TCP_PORT); 
	s32Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(s32Socket ,SOL_SOCKET,SO_REUSEADDR,&s32Socket_opt_value,sizeof(int)) == -1)     
    {
        return (void *)(-1);
    }
    s32Rtn = bind(s32Socket, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
    if(s32Rtn < 0)
    {
        return (void *)(-2);
    }
    
    s32Rtn = listen(s32Socket, 5);  
    if(s32Rtn < 0)
    {
         return (void *)(-2);
    }
	nAddrLen = sizeof(struct sockaddr_in);
	//int nSessionId = 1000;
    while ((s32CSocket = accept(s32Socket,(struct sockaddr*)&addrAccept,(socklen_t *)&nAddrLen)) >= 0)
    {
		printf("<<<<app Client %s Connected...\n", inet_ntoa(addrAccept.sin_addr));

		int nMaxBuf = 1024;
		if(setsockopt(s32CSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nMaxBuf, sizeof(nMaxBuf)) == -1){
			printf("RTSP:!!!!!! Enalarge socket sending buffer error !!!!!!\n");
		}

        for(i=0;i<MAX_APP_CLIENT;i++)
		{                       
			if(g_appclients[i].status == APPCLIENT_IDLE)
			{
				memset(&g_appclients[i],0,sizeof(APP_CLIENT));
				g_appclients[i].index = i;
				g_appclients[i].socket = s32CSocket;
				g_appclients[i].status = APPCLIENT_CONNECTED ;

                g_appclients[i].iOverTimeCnt = 0;
                  
				strcpy(g_appclients[i].IP,inet_ntoa(addrAccept.sin_addr));
				pToken = get_token_string(32);
				strcpy(g_appclients[i].token,pToken);
				free(pToken);
				pToken = NULL;
				
							
				//struct sched_param sched;
				//sched.sched_priority = 1;
				//to return ACKecho
				pthread_create(&(g_appclients[i].ClientMsgThreadID), NULL, AppClientMsg, &g_appclients[i]);
				//pthread_setschedparam(threadIdlsn,SCHED_RR,&sched);

				bAdd = TRUE;
				break;
			}
		}
		if(bAdd == FALSE)
		{
			close(s32CSocket);
		}
    }

	printf("----- AppServerListen() Exit !! \n");
	
	return NULL;
}


static int cdr_get_device_SN(char *pDeviceSN)
{ 
    struct ifreq ifreq; 
    int sock = 0; 
    unsigned	char strMAC[13] = {0};

    //printf("cdr_get_device_SN\n");
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
	{
		printf("%s socket error\n",__FUNCTION__);
		return   2;     
    }
    strcpy(ifreq.ifr_name,"wlan0"); 
	
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0)
	{
		printf("%s ioctl SIOCGIFHWADDR error\n",__FUNCTION__);
		return   3; 
    }
    
	sprintf(strMAC,"%02X%02X%02X%02X%02X", 
            (unsigned char)ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned char)ifreq.ifr_hwaddr.sa_data[2], 
            (unsigned char)ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned char)ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned char)ifreq.ifr_hwaddr.sa_data[5]); 
	memcpy(pDeviceSN,strMAC,10);

    //printf("cdr_get_device_SN %s\n",pDeviceSN);
    return 0; 
}

void cdr_app_service_init(void)
{
    char chTmp[30] = {0};
	time_t curTime;
	struct tm *ptm = NULL;
	
	pthread_t AppServerThreadID = 0;
	pthread_t ClientAliveThreadID = 0;	
	pthread_t AckClientThreadID = 0;

	memset(g_appclients,0,sizeof(APP_CLIENT)*MAX_APP_CLIENT);
	
	pthread_create(&AppServerThreadID, NULL, AppServerListen, NULL);
    pthread_create(&ClientAliveThreadID, NULL, AppClientAlive, NULL);
	pthread_create(&AckClientThreadID, NULL, AckClientPro, NULL);

    cdr_system("mkdir -p /mnt/mmc/tmp");
    get_system_cfg_from_xml();

    cdr_system("mkdir -p /mnt/mmc/log");
    cdr_system("mkdir -p /mnt/mmc/GPSTrail");
    
    snprintf(chTmp,sizeof(chTmp),"%s %s",__DATE__,__TIME__);
    update_hw_to_xml("cdrSystemInfomation","cdrBuildTime",chTmp,1);

	update_hw_to_xml("cdrSystemInfomation","cdrSoftwareVersion",CDR_FW_VERSION,1);
	update_hw_to_xml("cdrSystemInfomation","cdrHardwareVersion",CDR_HW_VERSION,1);	

    memset(chTmp,0,sizeof(chTmp));   
    if(0 == cdr_get_device_SN(chTmp))
    {
    	update_hw_to_xml("cdrSystemInfomation","cdrDeviceSN",chTmp,1);//cdrDeviceSN
    }

	//添加系统开机时间保存功能
	time(&curTime);
	ptm = gmtime(&curTime);
	memset(chTmp,0x00,30);
	sprintf(chTmp, "%04d%02d%02d%02d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	update_hw_to_xml("systemstarttime",NULL,chTmp,0);

	cdr_add_log(eSTART_CDR, NULL,NULL);    
}


//get system cfg params.
void get_system_cfg_from_xml(void)
{
	
}


void printf_protocol_info(CDR_PROTOCOL proData)
{
#if 0
	printf("\r\n");
	printf("*****************************************************\r\n")	;	
	char ch[200];
	memset(ch,0x00,200);
	memcpy(ch,proData.proheader.HeadFlag,4);
	printf("HeadFlag	:%s \r\n",ch);
	printf("CmdID		:%02x \r\n",proData.proheader.CmdID);
	printf("MsgSN		:%04d \r\n",proData.proheader.MsgSN);
	printf("VersionFlag	:%02x%02x \r\n",proData.proheader.VersionFlag[0],proData.proheader.VersionFlag[1]);
	
	memset(ch,0x00,200);
	memcpy(ch,proData.proheader.Token,32);
	printf("Token		:%s \r\n",ch);
	printf("MsgLenth	:%04d \r\n",proData.proheader.MsgLenth);	
	if(proData.proheader.MsgLenth > 0)
	{
		memset(ch,0x00,200);
		memcpy(ch,proData.MsgBody,proData.proheader.MsgLenth);
		printf("MsgBody	:%s \r\n",ch);	

		printf("%c %c %02x %02x \n",ch[proData.proheader.MsgLenth-2],ch[proData.proheader.MsgLenth-1],ch[proData.proheader.MsgLenth],ch[proData.proheader.MsgLenth+1]);
		printf("-->%02X %02X %02x %02x \n",ch[proData.proheader.MsgLenth-2],ch[proData.proheader.MsgLenth-1],ch[proData.proheader.MsgLenth],ch[proData.proheader.MsgLenth+1]);
	}

	printf("VerifyCode	:%04x \r\n",proData.VerifyCode);
	
	printf("*****************************************************\r\n");
#endif	
}



void pack_ack_packet(int clientIndex,int cmdid,char* msgBody,int msglen)
{
	CDR_PROTOCOL proTmp;
    
	memset(&proTmp,0x00,sizeof(CDR_PROTOCOL));
	memcpy(&proTmp.proheader.HeadFlag,"EYES",4);
	proTmp.proheader.CmdID = cmdid;
	proTmp.proheader.MsgSN = htons(g_appclients[clientIndex].proRecvBuffer.proheader.MsgSN);
	proTmp.proheader.VersionFlag[0] = (CDR_VERSION >> 8)&0xFF;
	proTmp.proheader.VersionFlag[1] = CDR_VERSION&0xFF;
	
	if(cmdid == CDR_login && g_appclients[clientIndex].loginflag == 1)
	{
		memcpy(&(proTmp.proheader.Token),g_appclients[clientIndex].token,32);	
	}
	proTmp.proheader.MsgLenth = msglen;
	memcpy(proTmp.MsgBody,msgBody,msglen);		
	//proTmp.VerifyCode = get_crc16((char*)&proTmp,sizeof(CDR_PROTOCOL_HEADER)+msglen);	
	memcpy(&g_appclients[clientIndex].proSendBuffer,&proTmp,sizeof(CDR_PROTOCOL));
}

int cdr_get_user_passwd(unsigned char *pucUserPasswd)
{
    unsigned char ucUserPasswdTemp[512] = {0};

    sprintf(ucUserPasswdTemp,"username:%s&passwd:%s\0",g_cdr_systemconfig.name,g_cdr_systemconfig.password);  //"username:EKAKA1&passwd:1234567"
    memcpy(pucUserPasswd,ucUserPasswdTemp,strlen(ucUserPasswdTemp));
	
    return 0;
}

//登录命令处理函数.
void app_cmd_login(int clientIndex)
{
    unsigned char ucUserPasswd[60] = {'\0'};
    unsigned char ucUserPasswd2[60] = {'\0'};

    cdr_get_user_passwd(ucUserPasswd);

    strncpy(ucUserPasswd2,&g_appclients[clientIndex].proRecvBuffer.MsgBody, strlen(ucUserPasswd));

	//if(strncmp(USERNAMEANDPASSWD,&g_appclients[clientIndex].proRecvBuffer.MsgBody,strlen(USERNAMEANDPASSWD)) == 0)
    if(strncmp(ucUserPasswd,&g_appclients[clientIndex].proRecvBuffer.MsgBody,strlen(ucUserPasswd)) == 0)
	{
		//登录成功
		g_appclients[clientIndex].loginflag = 1;
		pack_ack_packet(clientIndex,CDR_login,LOGIN_ACK_BODY,strlen(LOGIN_ACK_BODY));
	}
	else
	{
		//登录失败
		g_appclients[clientIndex].loginflag = 0;
		pack_ack_packet(clientIndex,CDR_login,CDR_RETURN_LOGINERROR,strlen(CDR_RETURN_LOGINERROR));		
	}
}
	



void app_cmd_pic_index_list(int clientIndex)
{
	//1.更新STR_INDEXPIC 文件
	update_index_list_xml();

	//ack packet.
	pack_ack_packet(clientIndex,CDR_pic_index_list,STR_INDEXPIC,strlen(STR_INDEXPIC));
}

void app_cmd_get_mp4_list(int clientIndex)
{
	//1.更新STR_MP4LIST 文件
	update_mp4_list_xml();
	//ack packet.
	pack_ack_packet(clientIndex,CDR_get_mp4_list,STR_MP4LIST,strlen(STR_MP4LIST));
}

void app_cmd_stop_rec(int clientIndex)
{
	//1.判断开启还是关闭.
	
	//ack packet.
	pack_ack_packet(clientIndex,CDR_stop_rec,CDR_RETURN_OK,strlen(CDR_RETURN_OK));
}

int cdr_mp4cut_finish(void* info)
{
	//printf("%s %d : %s \n",__FUNCTION__,__LINE__,info);

	char *p = strrchr(info,'/');
	p++;


	//Gsensor 生成的mp4 cpy一份到/mnt/mmc/tmp
	if(strstr(info,"GVIDEO"))
	{
		char cmd[100];
		sprintf(cmd,"cp -f %s %s/%s",info,MP4CUTOUTPATH,p);
		cdr_system(cmd);		
		cdr_system("sync");
	}

	ack_get_tmvideo_finish(p);
	
	return 0;
}

//获取某一时间点的视频.
void app_cmd_get_tmvideo(int clientIndex)
{
	//1..
	int nRet = 0;		
	char strTm[20];
	int len = 0;
	memset(strTm,0x00,sizeof(strTm));		
	sscanf(g_appclients[clientIndex].proRecvBuffer.MsgBody, "%[0-9]_%d", strTm,&len);
	nRet = cdr_read_mp4_ex(strTm,len,MP4CUTOUTPATH,cdr_mp4cut_finish);
	if(nRet == -1)
	{
		pack_ack_packet(clientIndex,CDR_get_tmvideo,CDR_RETURN_PARAMERROR,strlen(CDR_RETURN_PARAMERROR));
		send_ack_data(clientIndex);	
	}	
}


//拍照.
void app_cmd_pic_capture(int clientIndex)
{
	//1.拍照
	//char bodyMsg[30];
	//int bodylen = 20;
	cdr_capture_jpg(0);

	cdr_AssociatedVideo(g_cdr_systemconfig.photoWithVideo,CDR_CUTMP4_EX_TIME);
#if 0	

	g_cdr_systemconfig.photoWithVideo = 1;

	//Associated video 
	if(g_cdr_systemconfig.photoWithVideo == 1 &&
		g_cdr_systemconfig.recInfo.nPicStopFlag == 0)
	{		
		g_cdr_systemconfig.recInfo.nPicStopFlag = 1;
		cdr_capture_qcif(1);	
	}
	if (g_cdr_systemconfig.recInfo.nPicStopFlag == 2)
	{
		g_cdr_systemconfig.recInfo.nPicStopFlag = 1;
	}
#endif	
	//ack packet.
	//pack_ack_packet(clientIndex,CDR_pic_capture,bodyMsg,bodylen);
}



//获取拍照列表
void app_cmd_get_capture_list(int clientIndex)
{
	//1.判断开启还是关闭.
	update_jpg_list_xml();

	//ack packet.
	pack_ack_packet(clientIndex,CDR_get_capture_list,STR_JPGLIST,strlen(STR_JPGLIST));	
}

//SD卡信息
void app_cmd_get_sd_info(int clientIndex)
{
	//1.更新SD卡相关.STR_SDINFO	
	update_sd_info_xml();

	//ack packet.
	pack_ack_packet(clientIndex,CDR_get_sd_info,STR_SDINFO,strlen(STR_SDINFO));	
}


//SD卡信息
void app_cmd_del_file(int clientIndex)
{
	//1.删除文件..
	char chDelname[256];	
	sprintf(chDelname,"/mnt/mmc/%s",g_appclients[clientIndex].proRecvBuffer.MsgBody);
	if (0 == access(chDelname, W_OK))
	{
		if(0 == remove(chDelname))
			pack_ack_packet(clientIndex,CDR_del_file,CDR_RETURN_OK,strlen(CDR_RETURN_OK));	
		else
			pack_ack_packet(clientIndex,CDR_del_file,CDR_RETURN_ERROR,strlen(CDR_RETURN_ERROR));	

		return;			
	}
	//ack packet.
	pack_ack_packet(clientIndex,CDR_del_file,CDR_RETURN_FILENOTEXIST,strlen(CDR_RETURN_FILENOTEXIST));	
}


//format sd.
void app_cmd_format_sd(int clientIndex)
{
	//1.format sd..
	cdr_system("touch /home/formatsd");
	//ack packet.
	pack_ack_packet(clientIndex,CDR_format_sd,CDR_RETURN_OK,strlen(CDR_RETURN_OK));	

	//系统自动重启.
	sleep(3);
	cdr_system("reboot");		
}


//获取系统信息.
void app_cmd_set_rec_time(int clientIndex)
{
	//1.Update system info.STR_SYSTEMINFO
	cdr_rec_settime(g_appclients[clientIndex].proRecvBuffer.MsgBody);
	//ack packet.
	pack_ack_packet(clientIndex,CDR_set_rec_time,CDR_RETURN_OK,strlen(CDR_RETURN_OK));	
}



//get 
void app_cmd_get_system_cfg(int clientIndex)
{
	//1.Update STR_SYSCFG
	update_system_cfg_xml();
	//ack packet.
	pack_ack_packet(clientIndex,CDR_get_system_cfg,STR_SYSCFG,strlen(STR_SYSCFG));	
}

//get 
void app_cmd_get_gps_list(int clientIndex)
{
	update_gps_list_cfg_xml();   
	//ack packet.
	pack_ack_packet(clientIndex,CDR_get_gps_list,STR_GPSCFG,strlen(STR_GPSCFG));	
}

void app_cmd_get_smartunit_id(int clientIndex)
{
	pack_ack_packet(clientIndex,CDR_get_smartunit_ID,pucSmartUnitID,strlen(pucSmartUnitID));	  
}

//set 
void app_cmd_set_system_cfg(int clientIndex)
{
	//1.Update STR_SYSCFG
	int nRet = 0;
	nRet = cdr_syscfg_set_pro(clientIndex);
	//ack packet.
	if(nRet == 0)
		pack_ack_packet(clientIndex,CDR_set_system_cfg,CDR_RETURN_OK,strlen(CDR_RETURN_OK));
	else
		pack_ack_packet(clientIndex,CDR_set_system_cfg,CDR_RETURN_PARAMERROR,strlen(CDR_RETURN_PARAMERROR));
	
}

void app_cmd_settime(int clientIndex)
{
	if(0 == update_system_time(g_appclients[clientIndex].proRecvBuffer.MsgBody,g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth))
	//ack packet.
		pack_ack_packet(clientIndex,CDR_system_settime,CDR_RETURN_OK,strlen(CDR_RETURN_OK));	
	else
		pack_ack_packet(clientIndex,CDR_system_settime,CDR_RETURN_PARAMVALUEERROR,strlen(CDR_RETURN_PARAMVALUEERROR));
	//system reboot flag.
}


int cdr_system_reset()
{
    printf("[%s %d] cdr_system_reset\n",__FUNCTION__,__LINE__);
    
    g_cdr_device_app_stop_flag = CDR_POWER_OFF;
	//复位到默认值
    cdr_system("cp -rf /usr/cdr_syscfg.xml /home/");
	cdr_system("cp -rf /usr/cdr_syscfg.xml /mnt/cfg/");
	cdr_system("sync");
    
    cdr_device_deinit();
    sleep(2);
    cdr_system("reboot");    
}

/*
首先会停止视频录制功能，然后释放相关资源，关闭wifi、umount SD卡，然后执行reboot命令重启系统。接收到命令后，将会断开APP和记录仪的wifi连接
*/
void app_cmd_system_reset(int clientIndex)
{
	//ack packet.
	pack_ack_packet(clientIndex,CDR_system_reset,CDR_RETURN_OK,strlen(CDR_RETURN_OK));	

    cdr_system_reset();
   
}


//
void app_cmd_sofeware_update(int clientIndex)
{
	//ack packet.
	//char chUpdateFileName[256],cmd[256];
	//memset(chUpdateFileName,0x00,sizeof(chUpdateFileName));
	//sprintf(chUpdateFileName,"/mnt/mmc/tmp/%s",g_appclients[clientIndex].proRecvBuffer.MsgBody);
	//printf("%d",strlen(g_appclients[clientIndex].proRecvBuffer.MsgBody));	
	if (0 == access("/mnt/mmc/tmp/update.zip", F_OK))	
	{
		//sprintf(cmd,"mv -f %s /mnt/mmc/cdrfcimage.zip",chUpdateFileName);
		//sprintf(cmd,"mv -f /mnt/mmc/tmp/update.zip /mnt/mmc/cdrfcimage.zip",chUpdateFileName);
		//cdr_system(cmd);
		cdr_system("mv -f /mnt/mmc/tmp/update.zip /mnt/mmc/cdrfcimage.zip");
		cdr_system("unzip /mnt/mmc/cdrfcimage.zip -d /mnt/mmc/");
		pack_ack_packet(clientIndex, CDR_sofeware_update, CDR_RETURN_OK, strlen(CDR_RETURN_OK));
		cdr_system("sync");
		sleep(2);
		cdr_system("reboot");	
	}
	else
	{
		pack_ack_packet(clientIndex, CDR_sofeware_update, CDR_RETURN_FILENOTEXIST, strlen(CDR_RETURN_FILENOTEXIST));
	}
	//system reboot flag.
}


void send_ack_data(int clientIndex)
{
	char chSendData[300];
	int len = 0;	
	CDR_PROTOCOL_HEADER proheadertmp;

	//header 部分.
	memcpy(&proheadertmp,&g_appclients[clientIndex].proSendBuffer.proheader,sizeof(CDR_PROTOCOL_HEADER));
	
	//MsgLenth 需要转位大端模式.
	proheadertmp.MsgLenth = htons(proheadertmp.MsgLenth);
	
	memcpy(&chSendData,&proheadertmp,sizeof(CDR_PROTOCOL_HEADER));
	len = sizeof(CDR_PROTOCOL_HEADER);
	
	//msg body
	if(g_appclients[clientIndex].proSendBuffer.proheader.MsgLenth > 0)
	{
		memcpy(&chSendData[len],&g_appclients[clientIndex].proSendBuffer.MsgBody,
			g_appclients[clientIndex].proSendBuffer.proheader.MsgLenth);
		
		len += g_appclients[clientIndex].proSendBuffer.proheader.MsgLenth;
	}

	//crc 	
	unsigned short crc = get_crc16(chSendData,len);	
	chSendData[len++] = (crc >> 8) & 0xFF;
	chSendData[len++] = (crc) & 0xFF;

	//printf("ack..................\r\n",len);
	printf_protocol_info(g_appclients[clientIndex].proSendBuffer);		


#if 0	
	int xx = 0;
	printf("send: ");
	for(xx = 0;xx< len;xx++)
	{
		printf("%02x ",chSendData[xx]);
		if(xx > 0 && xx % 8 == 0)
			printf("\n");		
	}
	printf("\n");
#endif


	//printf("send :%d \r\n",len);	
	int sendlen = 0;
	sendlen = send(g_appclients[clientIndex].socket, chSendData,len,0);
	//printf("sendlen:%d\r\n",sendlen);
	printf("clientIndex:%d Send:%d\n",clientIndex,sendlen);
	//send();	
}


int check_token(int clientIndex)
{
	int nRet = 0;

	//登录命令不需要判断token.
	if(g_appclients[clientIndex].proRecvBuffer.proheader.CmdID == CDR_login)
		return 1;

	if(memcmp(&g_appclients[clientIndex].proRecvBuffer.proheader.Token,g_appclients[clientIndex].token,32) == 0)
		return 1;
	
	return nRet;
}

//判断EYEC字符串.
int check_protocol_header(int clientIndex)
{
	int nRet = 0;
	if(memcmp(&g_appclients[clientIndex].proRecvBuffer.proheader.HeadFlag,"EYEC",4) == 0)
		return 1;
	return nRet;
}

//判断EYEC字符串.
int check_protocol_crc(char* pData,int len)
{
	char crc[2] = {0};
	unsigned short s = get_crc16(pData,len-2);	

	crc[0] = (s >> 8) & 0xFF;
	crc[1] = (s) & 0xFF;
	if(pData[len-2] == crc[0] && pData[len-1] == crc[1])	return 1;

	printf("crc   %02x %02x ",(s >> 8) & 0xFF,(s) & 0xFF);	
	printf("recv  %02x %02x ",pData[len-2],pData[len-1]);

	return 0;
}


void parse_app_cmd(char* pData,int len,int clientIndex)
{
	#pragma pack(1)

	if(len < sizeof(CDR_PROTOCOL_HEADER) || pData == NULL)
		return;
#if 0	
	int xx = 0;
	printf("recv: ");
	for(xx = 0;xx< len;xx++)
	{
		printf("%02x ",pData[xx]);
		if(xx > 0 && xx % 8 == 0)
			printf("\n");		
	}
	printf("\n");
#endif
	
	if(check_protocol_crc(pData,len) == 0)
	{
		pack_ack_packet(clientIndex,g_appclients[clientIndex].proRecvBuffer.proheader.CmdID,CDR_RETURN_CRCERROR,strlen(CDR_RETURN_CRCERROR));	
		send_ack_data(clientIndex);	
		return;
	}

	memset(&g_appclients[clientIndex].proRecvBuffer,0x00,sizeof(CDR_PROTOCOL));	
	memset(&g_appclients[clientIndex].proSendBuffer,0x00,sizeof(CDR_PROTOCOL));	
	memcpy(&g_appclients[clientIndex].proRecvBuffer,pData,sizeof(CDR_PROTOCOL));
	
	g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth = ntohs(g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth);
	g_appclients[clientIndex].proRecvBuffer.proheader.MsgSN = ntohs(g_appclients[clientIndex].proRecvBuffer.proheader.MsgSN);
	
	g_appclients[clientIndex].proRecvBuffer.MsgBody[g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth]   = 0x00;
	g_appclients[clientIndex].proRecvBuffer.MsgBody[g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth+1] = 0x00;	
	g_appclients[clientIndex].proRecvBuffer.VerifyCode = 0x00;	
	
	printf_protocol_info(g_appclients[clientIndex].proRecvBuffer);



	if(check_token(clientIndex) == 1)
	{
		;//printf("token ok...\r\n");
	}
	else
	{		
		pack_ack_packet(clientIndex,g_appclients[clientIndex].proRecvBuffer.proheader.CmdID,CDR_RETURN_AUTHERROR,strlen(CDR_RETURN_AUTHERROR));	
		send_ack_data(clientIndex);	
		return ;
	}

	//Not have sd .
	if(0 == cdr_get_sd_status())
	{
		pack_ack_packet(clientIndex,g_appclients[clientIndex].proRecvBuffer.proheader.CmdID,CDR_RETURN_NOTSD,strlen(CDR_RETURN_NOTSD));	
		send_ack_data(clientIndex);	
		return ;
	}	

	if(check_protocol_header(clientIndex) == 0 || g_appclients[clientIndex].proRecvBuffer.proheader.MsgLenth > MSGBODY_MAX_LEN)
	{
		pack_ack_packet(clientIndex,g_appclients[clientIndex].proRecvBuffer.proheader.CmdID,CDR_RETURN_ERROR,strlen(CDR_RETURN_ERROR));	
		send_ack_data(clientIndex);	
		return;
	}



	g_appclients[clientIndex].iOverTimeCnt = 0;
	printf("recv index:%d,CmdID:%d\n",clientIndex,g_appclients[clientIndex].proRecvBuffer.proheader.CmdID);
	switch(g_appclients[clientIndex].proRecvBuffer.proheader.CmdID)
	{
		case CDR_login:
			app_cmd_login(clientIndex);
			break;
		case CDR_keep_live:
             //g_appclients[clientIndex].iOverTimeCnt = 0;
			pack_ack_packet(clientIndex,CDR_keep_live,CDR_RETURN_OK,strlen(CDR_RETURN_OK));			
			break;	
		case CDR_pic_index_list:
			app_cmd_pic_index_list(clientIndex);
			break;
		case CDR_get_mp4_list:
			app_cmd_get_mp4_list(clientIndex);
			break;	
		case CDR_stop_rec	:
			app_cmd_stop_rec(clientIndex);
			break;		
		case CDR_get_tmvideo :
			app_cmd_get_tmvideo(clientIndex);
			return;
			//break;		
		case CDR_pic_capture :
			app_cmd_pic_capture(clientIndex);
			return;
			//break;		
		case CDR_get_capture_list :
			app_cmd_get_capture_list(clientIndex);
			break;
		case CDR_get_sd_info :
			app_cmd_get_sd_info(clientIndex);
			break;		
		case CDR_del_file	:
			app_cmd_del_file( clientIndex);
			break;	
		case CDR_format_sd 	:
			app_cmd_format_sd(clientIndex);
			//cdr_play_audio(CDR_AUDIO_FORMATSD,0);
			break;		
		case CDR_set_rec_time 	:
			app_cmd_set_rec_time( clientIndex);
			break;
		case CDR_get_system_cfg	:
			app_cmd_get_system_cfg(clientIndex);
			break;	
		case CDR_set_system_cfg	:
			app_cmd_set_system_cfg(clientIndex);
			break;	
		case CDR_system_settime:
			cdr_play_audio(CDR_AUDIO_SETTIMEOK,0);
			app_cmd_settime(clientIndex);
			break;
		case CDR_system_reset	://恢复出厂设置
			cdr_play_audio(CDR_AUDIO_RESETSYSTEM,0);
			app_cmd_system_reset(clientIndex);			
			break;	
		case CDR_sofeware_update 	:
			cdr_play_audio(CDR_AUDIO_SYSTEMUPDATE,0);
			app_cmd_sofeware_update(clientIndex);			
			break;	
         case CDR_get_gps_list:
             app_cmd_get_gps_list(clientIndex);
             break;
         case CDR_get_smartunit_ID:
            //获取406主机的id 返回
            app_cmd_get_smartunit_id(clientIndex);
            break;
		default:
			return;	
			//处理错误命令.
	}	
	send_ack_data(clientIndex);	
}


void ack_capture_update(char *jpgname)
{
	if(jpgname == NULL)	return;
	cdr_add_ackmsg(CDR_pic_capture,jpgname);
}

//获取截取的视频返回到app
void ack_get_tmvideo_finish(char *vUri)
{
	if(vUri == NULL)return;
	cdr_add_ackmsg(CDR_get_tmvideo,vUri);	
}

//should release mem after get token.
char* get_token_string(int tokenlen)
{
	char *str = "0123456789QWERTYUIOPLKJHGFDSAZXCVBNM<>~!@#$%^&*()_+";
	char *token = malloc(tokenlen+1);
	int i,len = strlen(str) -1;
	srand(time(NULL));
	memset(token,0x00,sizeof(token));
	for(i=0;i<tokenlen;i++)
	{
		token[i] = str[rand()%len];
	}
	return token;	
}

int cdr_appinterface_answer(int cmdtype,unsigned short extcmdlen,char *pEtxcmd)
{
	int nRet = 0;

#if 0
	CDR_PROTOCOL pAnswertm = {0};
	CDR_PROTOCOL_HEADER pHeaderAnswertmp = {0};
	int socksendlen = CMD_SOCK_LEN_MIN;
	

	//填充header.
	memcpy(&pHeaderAnswertmp.fix_header,FIX_HEADER_S,FIE_HEADER_LEN);	
	pHeaderAnswertmp.version = PROTOCOL_VERSION;
	memset(&pHeaderAnswertmp.token,0x00,sizeof(pHeaderAnswertmp.token));	
	memcpy(&pAnswertm->pro_header,pHeaderAnswertmp,sizeof(CDR_PROTOCOL_HEADER));	

	//填充cmd & extcmd.
	pAnswertm->cmd = cmdtype;
	pAnswertm->extdata_len= extcmdlen;
	
	if(extcmdlen>0)
	{
		memcpy(&pAnswertm.cmd_extdata,pEtxcmd,extcmdlen);	
		socksendlen += extcmdlen;
	}

	//send data.
	//send();

#endif
	return nRet;
}






#define XML_STR_HEADER "<?xml VersionFlag=\"1.0\" encoding=\"utf-8\" ?>"

void get_xml_updatetime(char *pstr)
{
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};
	time(&curTime);
	ptm = gmtime(&curTime);
	memset(tmp,0x00,256);
	sprintf(tmp, "<xmlUpdateTime>%04d%02d%02d%02d%02d%02d</xmlUpdateTime>", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pstr, tmp);	
}

/*
<?xml VersionFlag="1.0" encoding="utf-8" ?> 
<cdrMp4>
	<xmlUpdateTime>20160608095700</xmlUpdateTime>
	<mp4>
	  <index>0</index> 
	  <fileName>201606041408.mp4</fileName> 
	</mp4>
</cdrMp4>
*/
int update_mp4_list_xml(void)
{
	cdr_system("mkdir -p /mnt/mmc/tmp");
	char chTmp[200];
	//system("touch -p /mnt/mmc/tmp/mp4_list.xml");
	FILE *pFile = NULL;
	pFile = fopen(XMLFILE_MP4_LIST,"wb");
	if(pFile == NULL)
	{
		return -1;
	}

	fwrite("<cdrMp4>\n",strlen("<cdrMp4>\n"),1,pFile);
	//fflush(pFile);		
	memset(chTmp,0x00,sizeof(chTmp));
	get_xml_updatetime(chTmp);
	fwrite(chTmp,strlen(chTmp),1,pFile);
#if 1	
	DIR *dir;
	struct dirent *ptr;
	dir = opendir("/mnt/mmc/VIDEO");
	char chName[256];
	int index = 0;
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;

		//这个地方判断是否为一个已经录制好的文件.
		//printf("ptr->d_reclen:%d,ptr->d_name:%s\r\n",ptr->d_reclen,ptr->d_name);

		if(NULL == strchr(ptr->d_name,'_'))
			continue;
		
	
		fwrite("<mp4>\n",strlen("<mp4>\n"),1,pFile);

		//0.index
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<index>%d</index>\n", index++);	
		fwrite(chName,strlen(chName),1,pFile);
		
		//1.file name
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<fileName>%s</fileName>\n", ptr->d_name);	
		fwrite(chName,strlen(chName),1,pFile);
#if 0
		//2.createTime
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<createTime>201606041408</createTime>\n");	
		fwrite(chName,strlen(chName),1,pFile);


		//3.createTime
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<streamDuration>180000</streamDuration>\n");	
		fwrite(chName,strlen(chName),1,pFile);
		
		//4.start time
				memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<streamDuration>180000</streamDuration>\n");	
		fwrite(chName,strlen(chName),1,pFile);

		//5.end time.
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<streamDuration>180000</streamDuration>\n");	
		fwrite(chName,strlen(chName),1,pFile);
		
#endif		
		fwrite("</mp4>\n",strlen("</mp4>\n"),1,pFile);		
	}
	closedir(dir);
#endif

	fwrite("</cdrMp4>\n",strlen("</cdrMp4>\n"),1,pFile);
	fflush(pFile);
	fclose(pFile);
	return 0;
}

/*

tmp/jpg_list.xml


<?xml VersionFlag="1.0" encoding="utf-8" ?> 
<cdJpg>
    <xmlUpdateTime>20160608095700</xmlUpdateTime>
    <jpg>
        <index>0</index>
        <fileName>201606041408.jpg</fileName>
    </jpg>
    <jpg>
        <index>1</index>
        <fileName>201606041508.jpg</fileName>
    </jpg>
    <jpg>
        <index>2</index>
        <fileName>201606041608.jpg</fileName>
    </jpg>
</cdJpg>
*/

int update_jpg_list_xml(void)
{
	cdr_system("mkdir -p /mnt/mmc/tmp");
	char chTmp[200];
	//system("touch -p /mnt/mmc/tmp/mp4_list.xml");
	FILE *pFile = NULL;
	pFile = fopen(XMLFILE_JPG_LIST,"wb");
	if(pFile == NULL)
	{
		return -1;
	}

	//fwrite(XML_STR_HEADER,strlen(XML_STR_HEADER),1,pFile);
	//fflush(pFile);	

	fwrite("<cdJpg>\n",strlen("<cdJpg>\n"),1,pFile);
	//fflush(pFile);
		
	memset(chTmp,0x00,sizeof(chTmp));
	get_xml_updatetime(chTmp);
	fwrite(chTmp,strlen(chTmp),1,pFile);

	DIR *dir;
	struct dirent *ptr;
	dir = opendir("/mnt/mmc/PHOTO");
	char chName[256];
	int index = 0;
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;
		//Remove xxx.jgp_tmp file.
		if(strstr(ptr->d_name,"tmp") != NULL)
			continue;

		//去掉没有.的文件.
		if(strchr(ptr->d_name,'.') == NULL)
			continue;

		//Remove xxx.jgp_tmp file.
		//if(strstr(ptr->d_name,"pre") != NULL)
		//	continue;
			
		fwrite("<jpg>\n",strlen("<jpg>\n"),1,pFile);

		//0.index
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<index>%d</index>\n", index++);	
		fwrite(chName,strlen(chName),1,pFile);
		
		//1.file name
		memset(chName, 0, sizeof(chName));	
		snprintf(chName, 256, "<fileName>%s</fileName>\n", ptr->d_name);	
		fwrite(chName,strlen(chName),1,pFile);
		
		fwrite("</jpg>\n",strlen("<jpg>\n"),1,pFile);		
	}
	closedir(dir);


	fwrite("</cdJpg>\n",strlen("</cdJpg>\n"),1,pFile);
	fflush(pFile);
	fclose(pFile);
	return 0;
}


/*
<jpg>
        <index>0</index>
        <fileName>20160604140800.jpg</fileName>
</jpg>

*/

int update_index_list_xml(void)
{
	cdr_system("mkdir -p /mnt/mmc/tmp");
	char chTmp[200];
	//system("touch -p /mnt/mmc/tmp/mp4_list.xml");
	FILE *pFile = NULL;
	pFile = fopen(XMLFILE_INDEX_LIST,"wb");
	if(pFile == NULL)
	{
		return -1;
	}

	//fwrite(XML_STR_HEADER,strlen(XML_STR_HEADER),1,pFile);
	//fflush(pFile);	

	fwrite("<cdrIndex>\n",strlen("<cdrIndex>\n"),1,pFile);
	//fflush(pFile);
		
	memset(chTmp,0x00,sizeof(chTmp));
	get_xml_updatetime(chTmp);
	fwrite(chTmp,strlen(chTmp),1,pFile);

	DIR *dir;
	struct dirent *ptr;
	dir = opendir("/mnt/mmc/INDEX");
	char chName[256];
	int index = 0;
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;

		if(strstr(ptr->d_name,"jpg"))	
		{
			fwrite("<jpg>\n",strlen("<jpg>\n"),1,pFile);

			//0.index
			memset(chName, 0, sizeof(chName));	
			snprintf(chName, 256, "<index>%d</index>\n", index++);	
			fwrite(chName,strlen(chName),1,pFile);
			
			//1.file name
			memset(chName, 0, sizeof(chName));	
			snprintf(chName, 256, "<fileName>%s</fileName>\n", ptr->d_name);	
			fwrite(chName,strlen(chName),1,pFile);
			
			fwrite("</jpg>\n",strlen("<jpg>\n"),1,pFile);	
		}
		if(strstr(ptr->d_name,"zip"))	
		{
			fwrite("<tar>\n",strlen("<tar>\n"),1,pFile);

			//0.index
			memset(chName, 0, sizeof(chName));	
			snprintf(chName, 256, "<index>%d</index>\n", index++);	
			fwrite(chName,strlen(chName),1,pFile);
			
			//1.file name
			memset(chName, 0, sizeof(chName));	
			snprintf(chName, 256, "<fileName>%s</fileName>\n", ptr->d_name);	
			fwrite(chName,strlen(chName),1,pFile);
			
			fwrite("</tar>\n",strlen("<tar>\n"),1,pFile);	
		}
	}
	closedir(dir);
	fwrite("</cdrIndex>\n",strlen("</cdrIndex>\n"),1,pFile);
	fflush(pFile);
	fclose(pFile);
	return 0;
}

/*

<?xml VersionFlag="1.0" encoding="utf-8" ?> 
<cdrSdInformation>
	<xmlUpdateTime>20160608095700</xmlUpdateTime>
	<size>30000</size>
	<used>10000</used>
	<free>20000</free>
	<mediaInformation>
		<firstRecord>201606070935.mp4</firstRecord>
		<lastRecord>201606080935.mp4</lastRecord>
		<recFileCount>100</recFileCount>
	</mediaInformation>
</cdrSdInformation>
*/

int cdr_get_strorage_info(void)
{
    const unsigned char *pVideoPath = "/mnt/mmc/VIDEO";
    const unsigned char *pProtectPath = "/mnt/mmc/PROTECT";
    const unsigned char *pPhotoPath = "/mnt/mmc/PHOTO";
    
    struct statfs statFS;

    if (statfs(SD_MOUNT_PATH, &statFS) == -1)//dev/mmcblk0p1
    {  
        printf("error, statfs failed !\n");
        return -1;
    }
    g_SdParam.allSize   = ((statFS.f_blocks/1024)*(statFS.f_bsize/1024));
    g_SdParam.leftSize  = (statFS.f_bfree/1024)*(statFS.f_bsize/1024); 
    g_SdParam.haveUse   = g_SdParam.allSize - g_SdParam.leftSize;    

    g_SdParam.iVideoSize = cdr_get_file_size(pVideoPath);
    g_SdParam.uiProtectSize = cdr_get_file_size(pProtectPath);
    g_SdParam.uiPhotoSize = cdr_get_file_size(pPhotoPath);
    g_SdParam.uiOtherSize = g_SdParam.allSize - g_SdParam.iVideoSize - g_SdParam.uiProtectSize - g_SdParam.uiPhotoSize-g_SdParam.leftSize;

    printf("allSize=%ld...leftSize=%ld...haveUse=%ld \n", 
        g_SdParam.allSize, g_SdParam.leftSize, g_SdParam.haveUse);

      
    printf("iVideoSize=%ld...uiProtectSize=%ld...uiPhotoSize=%ld uiOtherSize=%ld\n", 
        g_SdParam.iVideoSize, g_SdParam.uiProtectSize, g_SdParam.uiPhotoSize,g_SdParam.uiOtherSize);

    return 0;
}

static char chFirstFile[256] = {0};
static char chLastFile[256] = {0};
static int iFileCout = 0;

static void cdr_get_file_info()
{
	DIR *dir;
	struct dirent *ptr;
	dir = opendir("/mnt/mmc/VIDEO");

	memset(chFirstFile,0x00,sizeof(chFirstFile));
	memset(chLastFile,0x00,sizeof(chLastFile));
   	    
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;
		//这个地方判断是否为一个已经录制好的文件.
		//printf("ptr->d_reclen:%d,ptr->d_name:%s\r\n",ptr->d_reclen,ptr->d_name);
				
		if(NULL == strchr(ptr->d_name,'_'))
			continue;
		
		//printf("ptr->d_name:%s\r\n",ptr->d_name);
		
		if(chFirstFile[0] == 0x00)
		{
			snprintf(chFirstFile, sizeof(chFirstFile), "%s",ptr->d_name);	
		}
		
		if(chLastFile[0] == 0x00)
		{
			memset(chLastFile,0x00,sizeof(chLastFile));
			snprintf(chLastFile, sizeof(chLastFile), "%s",ptr->d_name);	
		}

		iFileCout ++;

		if(memcmp(ptr->d_name, chFirstFile, 256) < 0)
		{
			memset(chFirstFile,0x00,sizeof(chFirstFile));
			snprintf(chFirstFile, sizeof(chFirstFile), "%s",ptr->d_name);	
		}

		if(memcmp(ptr->d_name, chLastFile, 256) > 0)
		{
			memset(chLastFile,0x00,sizeof(chLastFile));
			snprintf(chLastFile, sizeof(chLastFile), "%s",ptr->d_name);	
		}
				
	}
	closedir(dir);

}


static unsigned long cdr_get_file_size(const char *path)  
{          
    unsigned char ucFileNameArr[40] = {0};
    unsigned long long filesize = 0;  
    struct stat statbuff;      
    
    DIR *dir;
	struct dirent *ptr;

    if(access(path, R_OK) == -1){       
       return 0;
    }   
    
	dir = opendir(path);
    if(dir==NULL){
       return 0;
    }
	
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)continue; 

        sprintf(ucFileNameArr,"%s/%s",path,ptr->d_name);
        if(stat(ucFileNameArr, &statbuff) < 0){  
          continue;
        }else{  
          filesize = filesize + statbuff.st_size;     
        }  		
	}
	closedir(dir);

    return filesize>>20;
} 

int update_sd_info_xml()
{    
    unsigned char ucTimeTemp[30] = {0};
    const unsigned char *ucFileName = "/mnt/mmc/tmp/cdr_sdinfo.xml";
    
    FILE *fp = NULL;
    mxml_node_t *tree,*cdrSDInformation,*mediaInfomation;   
   
    cdr_system("mkdir -p /mnt/mmc/tmp");

    cdr_get_strorage_info();
	cdr_get_file_info();    
    cdr_log_get_time(ucTimeTemp);
    
    tree = mxmlNewXML("1.0");            
    cdrSDInformation = mxmlNewElement(tree,"cdrSDInformation");

    cdr_add_xml_txt_elem(cdrSDInformation,"xmlUpdateTime",ucTimeTemp);
    
    cdr_add_xml_int_elem(cdrSDInformation,"size",g_SdParam.allSize);

    cdr_add_xml_int_elem(cdrSDInformation,"use",g_SdParam.haveUse);

    cdr_add_xml_int_elem(cdrSDInformation,"free",g_SdParam.leftSize);

    cdr_add_xml_int_elem(cdrSDInformation,"recVideoSize",g_SdParam.iVideoSize);

    cdr_add_xml_int_elem(cdrSDInformation,"protectVideoSize",g_SdParam.uiProtectSize);

    cdr_add_xml_int_elem(cdrSDInformation,"photoSize",g_SdParam.uiPhotoSize);
    
    cdr_add_xml_int_elem(cdrSDInformation,"other",g_SdParam.uiOtherSize);

    mediaInfomation = mxmlNewElement(cdrSDInformation, "mediaInfomation");
    cdr_add_xml_txt_elem(mediaInfomation,"firstRecord",chFirstFile);
    cdr_add_xml_txt_elem(mediaInfomation,"lastRecord",chLastFile);
    cdr_add_xml_int_elem(mediaInfomation,"recFileCount",iFileCout);

	fp = fopen(ucFileName, "w");
    if(fp==NULL){
        printf("open file fails\n");
        return -1;
    }
	mxmlSaveFile(tree,fp,MXML_NO_CALLBACK);
	fflush(fp);
	fclose(fp);
    fp = NULL;
    mxmlDelete(tree);
    
    return 0;      
}

int get_trip_time(unsigned char *pucFileName)
{
     int iTripTime = 0;
     unsigned char ucTripTime[8] = {0};//28800s = 8h 
     int i = 0;
     int len = strlen(pucFileName);
     unsigned char *ucHeader = pucFileName;
     
     //for(i = 0; i < (strlen(pucFileName));i++)
     for(i = 0; i < len;i++)        
     {
       if(*pucFileName == 'z')
       {
        printf("i:%d\n",i);
        break;
       }
       pucFileName++;
     }  
     pucFileName = ucHeader;
     memcpy(ucTripTime,pucFileName+19,i-19-1);//gps_19700114205542_103.zip
     ucTripTime[i-19] = '\0';
     printf("ucTripTime:%s\n",ucTripTime);
     iTripTime = atoi(ucTripTime);
     printf("iTripTime:%d\n",iTripTime);
     return iTripTime;
}

/*
1,create  gps list xml
2,数据格式对不对
3,数值对不对
4,存入的目录是 /mnt/mmc/tmp/
5,kaka pc soft ip 获取有问题
*/
int update_gps_list_cfg_xml(void)
{
    unsigned char ucGpFileName[20] = {0};
    unsigned int uiTemp = 0;
    unsigned char ucTemp[12] = {0};   
    char chTmp[200] = {0};
	char chName[256] = {0};
    FILE *pFile = NULL;

	pFile = fopen(XMLFILE_GPS_LIST,"wb");
	if(pFile == NULL)
	{
         printf("[%s,%d] fopen XMLFILE_GPS_LIST fails\n",__FUNCTION__,__LINE__);
		return -1;
	}

	fwrite("<cdrGps>\n",strlen("<cdrGps>\n"),1,pFile);	
	memset(chTmp,0x00,sizeof(chTmp));
	get_xml_updatetime(chTmp);//<xmlUpdateTime>20160608095700</xmlUpdateTime>
	fwrite(chTmp,strlen(chTmp),1,pFile);
    
#if 1
    /*
    <gps>
       <index>0</index>	
       <fileName>gps_20160608080603.gp</fileName>
    </gps>
    只有一条，保存最新的行进中那条，若完成，则删除
    */
    fwrite("<gps>\n",strlen("<gps>\n"),1,pFile);  

    //0.index
    memset(chName, 0, sizeof(chName));	
    snprintf(chName, 256, "<index>%d</index>\n", 0);	
    fwrite(chName,strlen(chName),1,pFile);

    //1.file name
    gp_get_file_name(ucGpFileName);
    memset(chName, 0, sizeof(chName));	
    snprintf(chName, 256, "<fileName>%s</fileName>\n", ucGpFileName);//当前的文件名	
    fwrite(chName,strlen(chName),1,pFile);

    fwrite("</gps>\n",strlen("</gps>\n"),1,pFile);  
#endif
    
#if 1
    /*
    <zip>
    	<index>0</index>		
    	<fileName>gps_20160608070603_3401.zip</fileName>
    	<startMileage>10000.2</startMileage>
    	<endMileage>10003.3</endMileage>
    	<tirpMileage>3.1</tirpMileage>
    	<tirpTime>108</tirpTime>
    </zip>
    */
	DIR *dir;
	struct dirent *ptr;
	dir = opendir("/mnt/mmc/GPSTrail");//此目录可能要改

	int index = 0;
	while (dir && (ptr = readdir(dir)) != NULL)
	{
        if (ptr->d_type != 8)  continue;

        if(strstr(ptr->d_name,"gps_")==NULL) continue;
        if(strstr(ptr->d_name,".zip")==NULL) continue;

        fwrite("<zip>\n",strlen("<zip>\n"),1,pFile);

        //0.index
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<index>%d</index>\n", index++);	
        fwrite(chName,strlen(chName),1,pFile);

        //1.file name
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<fileName>%s</fileName>\n", ptr->d_name);	
        fwrite(chName,strlen(chName),1,pFile);

        //2.startMileage
        uiTemp = get_cdr_start_milage();
        memset(ucTemp,0,sizeof(ucTemp));
        sprintf(ucTemp,"%d",uiTemp);
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<startMileage>%s</startMileage>\n", ucTemp);	
        fwrite(chName,strlen(chName),1,pFile);

        //3.endMileage
        uiTemp = get_cdr_stop_milage();
        memset(ucTemp,0,sizeof(ucTemp));
        sprintf(ucTemp,"%d",uiTemp);
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<endMileage>%s</endMileage>\n", ucTemp);	
        fwrite(chName,strlen(chName),1,pFile);

        //4.tirpMileage
        uiTemp = get_cdr_trip_milage(get_cdr_stop_milage());
        memset(ucTemp,0,sizeof(ucTemp));
        sprintf(ucTemp,"%d",uiTemp);
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<tirpMileage>%s</tirpMileage>\n", ucTemp);	
        fwrite(chName,strlen(chName),1,pFile);

        //5.tirpTime
        int itemp = get_trip_time(ptr->d_name);
        //int itemp = get_trip_time(ptr->d_name);//get_cdr_trip_sec(get_cdr_stop_sec());
        memset(ucTemp,0,sizeof(ucTemp));
        sprintf(ucTemp,"%d",itemp);
        memset(chName, 0, sizeof(chName));	
        snprintf(chName, 256, "<tirpTime>%s</tirpTime>\n", ucTemp);	
        fwrite(chName,strlen(chName),1,pFile);

        fwrite("</zip>\n",strlen("</zip>\n"),1,pFile);		
	}
	if(dir)
		closedir(dir);
#endif
	fwrite("</cdrGps>\n",strlen("</cdrGps>\n"),1,pFile);
	fflush(pFile);
	fclose(pFile);
	return 0;
}

int update_system_cfg_xml(void)
{
	cdr_system("cp /home/cdr_syscfg.xml /mnt/mmc/tmp/");
}


//比较系统当前时间和设置时间差异.
int compare_system_time(char *pTm)
{
	time_t settm = 0;
	settm = _strtotime(pTm);	
	time_t curTime;
	time(&curTime);
	printf("%d %d %d\n",settm,curTime,ABS(settm-curTime));
	return ABS(settm-curTime);
}
int update_system_time(char *ptimestr,int len)
{
	printf("update_system_time:%s,%d\n",ptimestr,len);
	
	int nRet = 0;
	if(len < 12)
	{
		return -1;
	}

	//系统时间误差1分钟内.不矫正.
	if(compare_system_time(ptimestr) < 60)
	{
		printf("not need update time\n");
		return 0;
	}
	
	
	char chTmp[50] = {0},chTm[20] = {0};
	memcpy(chTm,ptimestr,12);
	chTm[12] = 0x00;
	
	snprintf(chTmp,sizeof(chTmp),"date -s %s ",chTm);
	printf("%s\r\n",chTmp);
	//system("ls -l");
	cdr_system(chTmp);	

	//把系统时间设定为rtc时间
	cdr_system("/home/cdr_rtc_app 1");	
	
	return nRet;
}


//system init 更新数据到xml.
int update_hw_to_xml(char* rootNode,char* childNode,char* value,int childflag)
{
	FILE *fp;
	mxml_node_t *tree;
	fp = fopen("/home/cdr_syscfg.xml", "r");
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);
	printf("%s %s %s %s %d\r\n",__FUNCTION__,rootNode,childNode,value,childflag);
	
	if(rootNode == NULL || value == NULL || strlen(value) <= 0)
	{
		return -1;
	}
	
	mxml_node_t *node;
	node = mxmlFindElement(tree, tree, rootNode,NULL, NULL,MXML_DESCEND);
	if(node == NULL)
	{
		printf("Not found root node\r\n");
		mxmlRelease(tree);		
		return -1;
	}
	if(childflag)
	{
		node = mxmlFindElement(node, node, childNode,NULL, NULL,MXML_DESCEND);
		if(node == NULL)
		{
			printf("Not found childNode\r\n");
			mxmlRelease(tree);
			return -1;
		}
	}
	if(node)
	{
		mxml_node_t *nodeparent = NULL;
		nodeparent = mxmlGetParent(node);
		mxmlDelete(node);

		if(childflag){
			node = mxmlNewElement(nodeparent, childNode);
		}else{
			node = mxmlNewElement(nodeparent, rootNode);		
		}
		mxmlNewText(node, 0, value);		
		//printf("%s %s \r\n",rootNode,mxmlGetText(node,1));	
		//mxmlSetText(node,1,value);
		fp = fopen("/home/cdr_syscfg.xml", "w");
		mxmlSaveFile(tree,fp,MXML_NO_CALLBACK);
		fflush(fp);
		fclose(fp);		
	}	

	cdr_system("cp -rf /home/cdr_syscfg.xml /mnt/mmc/tmp/cdr_syscfg.xml");
	cdr_system("cp -rf /home/cdr_syscfg.xml /mnt/cfg/cdr_syscfg.xml");
	mxmlRelease(tree);
	
	return 0;
	
}


//更新数据到xml.
int save_cfg_to_xml(char* rootNode,char* childNode,char* value,int childflag)
{
	FILE *fp;
	mxml_node_t *tree;
	fp = fopen("/home/cdr_syscfg.xml", "r");
    if(fp==NULL){
       printf("open fails\n");
       return;
    }
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
    SAFE_CLOSE(fp);
    
	//printf("%s %s %s %s %d\r\n",__FUNCTION__,rootNode,childNode,value,childflag);
	
	if(rootNode == NULL || value == NULL)
	{
		return -1;
	}
	//不能修改只读项目.
	if(memcmp(rootNode,"rtspLive",strlen("rtspLive")) == 0 ||
		memcmp(rootNode,"rtspRecord",strlen("rtspRecord")) == 0 ||
		memcmp(rootNode,"cdrSystemInfomation",strlen("cdrSystemInfomation")) == 0 ||
		memcmp(rootNode,"cdrSystemSDFilePath",strlen("cdrSystemSDFilePath")) == 0 )
		{
			printf("cann't chage readonly itme...%s\r\n",rootNode);
			return -1;
		}
	
	
	mxml_node_t *node;
	node = mxmlFindElement(tree, tree, rootNode,NULL, NULL,MXML_DESCEND);
	if(node == NULL)
	{
		printf("Not found root node\r\n");
		mxmlRelease(tree);		
		return -1;
	}
	if(childflag)
	{
		node = mxmlFindElement(node, node, childNode,NULL, NULL,MXML_DESCEND);
		if(node == NULL)
		{
			printf("Not found childNode\r\n");
			mxmlRelease(tree);
			return -1;
		}
	}
	if(node)
	{
		//printf("%s %s \r\n",rootNode,mxmlGetText(node,0));	
		mxmlSetText(node,0,value);
		//printf("%s %s \r\n",rootNode,mxmlGetText(node,0));	        
		fp = fopen("/home/cdr_syscfg.xml", "w");
		mxmlSaveFile(tree,fp,MXML_NO_CALLBACK);
		fflush(fp);
		fclose(fp);		
	}	

	cdr_system("cp -rf /home/cdr_syscfg.xml /mnt/mmc/tmp/cdr_syscfg.xml");
	cdr_system("cp -rf /home/cdr_syscfg.xml /mnt/cfg/cdr_syscfg.xml");
	mxmlRelease(tree);
	
	return 0;
	
}


//更新数据到xml.
int read_cfg_from_xml(char* rootNode,char* childNode,char* value,int childflag)
{
	FILE *fp;
	mxml_node_t *tree;
	fp = fopen("/home/cdr_syscfg.xml", "r");
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);
	printf("%s %s %s %s %d\r\n",__FUNCTION__,rootNode,childNode,value,childflag);
	
	if(rootNode == NULL)
	{
		return -1;
	}
	
	mxml_node_t *node;
	node = mxmlFindElement(tree, tree, rootNode,NULL, NULL,MXML_DESCEND);
	if(node == NULL)
	{
		printf("Not found root node\r\n");
		mxmlRelease(tree);		
		return -1;
	}
	if(childflag)
	{
		node = mxmlFindElement(node, node, childNode,NULL, NULL,MXML_DESCEND);
		if(node == NULL)
		{
			printf("Not found childNode\r\n");
			mxmlRelease(tree);
			return -1;
		}
	}
	if(node)
	{
		char *pText = NULL;
		pText = mxmlGetText(node,0);
		strcpy(value,pText);			
		printf("%s %s \r\n",rootNode,pText);
	}	

	mxmlRelease(tree);
	
	return 0;
	
}

int Set_VolumeRecordingSensitivity(int iIndex)
{
 
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    
     g_cdr_systemconfig.volumeRecordingSensitivity = iIndex;
   	
	unsigned int gain_mic;
	//Hi3516A 的 arg 取值范围为[0, 31]，按 1.5db 递增， 0 对应最小增益-16.5db， 31 对应最大增益 30db
     //gain_mic = g_cdr_systemconfig.volumeRecordingSensitivity*10;//[0 3]
     gain_mic = g_cdr_systemconfig.volumeRecordingSensitivity*5;//[0 16]
     
     if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
	{
		printf("ioctl err!\n");
	}

	//unsigned int gain_mac;
	//gain_mic = 0x07;
	gain_mic = g_cdr_systemconfig.volumeRecordingSensitivity*5;//[0 3]
	if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
	{
		printf("ioctl err!\n");
	} 
    close(fdAcodec);

    if(gain_mic == 0x00)Set_AI_MuteVolume(0x00);
    else Set_AI_MuteVolume(0x01);
  
  return 0;
}

int Set_VolumeCtrl(int iValueIndex)
{

    g_cdr_systemconfig.volume = iValueIndex;
    
     //理论范围[-121, 6] 
     //测试有效范围  [40-93, 99-93]
    if(iValueIndex != 0x00){
       iValueIndex = (iValueIndex+1)/2 + 49 ;// [0 99]
       iValueIndex = iValueIndex - 93;//[-93 6]
    }else{
       iValueIndex = -121;
    }
    
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
 
    	if (ioctl(fdAcodec, ACODEC_SET_OUTPUT_VOL, &iValueIndex))
	{
		printf("ioctl err!\n");
	} 
    
    close(fdAcodec);

    //Set_AI_MuteVolume(0x01);
    
   return 0;
}

//参数ucVolMuteFlag 0表示静音，为1开启音频
int Set_MuteVolume(unsigned char ucVolMuteFlag)
{
    
	ACODEC_VOL_CTRL vol_ctrl;
    
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    
    vol_ctrl.vol_ctrl_mute = 0x0;
    if(ucVolMuteFlag == 0x00)	vol_ctrl.vol_ctrl_mute = 0x1;//静音
	vol_ctrl.vol_ctrl = 0x00;
	if (ioctl(fdAcodec, ACODEC_SET_DACL_VOL, &vol_ctrl))//左声道输出音量控制
	{
		printf("ioctl err!\n");
	}
	
	if (ioctl(fdAcodec, ACODEC_SET_DACR_VOL, &vol_ctrl))
	{
		printf("ioctl err!\n");
	}

    close(fdAcodec);
    return 0;
}


//参数ai ucVolMuteFlag 0表示静音，为1开启音频
int Set_AI_MuteVolume(unsigned char ucVolMuteFlag)
{
    
	ACODEC_VOL_CTRL vol_ctrl;
    
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    
    vol_ctrl.vol_ctrl_mute = 0x0;
    if(ucVolMuteFlag == 0x00)	vol_ctrl.vol_ctrl_mute = 0x1;//静音
	vol_ctrl.vol_ctrl = 0x00;
	if (ioctl(fdAcodec, ACODEC_SET_ADCL_VOL, &vol_ctrl))//左声道输出音量控制
	{
		printf("ioctl err!\n");
	}
	
	if (ioctl(fdAcodec, ACODEC_SET_ADCR_VOL, &vol_ctrl))
	{
		printf("ioctl err!\n");
	}

    close(fdAcodec);
    return 0;
}

static unsigned char g_ucBootVoiceFlag = 0;
//开机提示音
int Set_BootVoiceCtrl(int iSw)
{
	if(iSw != 0 || iSw != 1)
		return -1;

	g_cdr_systemconfig.bootVoice = iSw;

	return 0;
}

int Get_BootVoiceCtrl()
{
	return g_cdr_systemconfig.bootVoice;
}

//---impl key ctrol----------------------------------------------------
int cdr_syscfg_set_pro_impl(char *pRoot,char *pChildNode,char *pValue,int childflag)
{
	if(pChildNode == NULL || pValue == NULL)
	{
		return -1;
	}
    int iValue = -1;


	if(strcmp("name",pRoot) == 0x00)
	{
		memset(g_cdr_systemconfig.name,0x00,256);
		memcpy(g_cdr_systemconfig.name,pValue,strlen(pValue));
	}
	if(strcmp("password",pRoot) == 0x00)
	{
		memset(g_cdr_systemconfig.password,0x00,256);
		memcpy(g_cdr_systemconfig.password,pValue,strlen(pValue));
	}
			
    if(strcmp("volumeRecordingSensitivity",pRoot) == 0x00)
    {
       iValue = atoi(pValue);
       Set_VolumeRecordingSensitivity(iValue);
    }
    if(strcmp("volume",pRoot)==0)
	{
	   iValue = atoi(pValue);
       Set_VolumeCtrl(iValue);
       //HI_MPI_AO_SetVolume();
    }
    if(strcmp("bootVoice",pRoot)==0)//开机提示音
	{
	   iValue = atoi(pValue);
       Set_BootVoiceCtrl(iValue);
    }

    //1080 720
    if(strcmp("videoRecord",pRoot)==0x00 && strcmp("type",pChildNode)==0x00)
    {
       iValue = atoi(pValue);
       cdr_mpp_set_video_resolution(iValue);
    }
    //16:9 2.4:1
    if(strcmp("videoRecord",pRoot)==0x00 && strcmp("mode",pChildNode)==0x00)
    {
       iValue = atoi(pValue);
       cdr_mpp_set_video_mode(iValue);
    }
    if(strcmp("videoRecord",pRoot)==0x00 && strcmp("rotate",pChildNode)==0x00)
    {
       iValue = atoi(pValue);
       cdr_mpp_set_video_rotate(iValue);    
    }

    if((strcmp("osd",pRoot)==0x00) && (strcmp("logo",pChildNode)==0x00))
    {
       iValue = atoi(pValue);

       g_cdr_systemconfig.sCdrOsdCfg.logo = iValue;
       
       cdr_osd_sw(0x02,iValue);
    }
    if((strcmp("osd",pRoot)==0x00) && (strcmp("time",pChildNode)==0x00))
    {          
       iValue = atoi(pValue);
       g_cdr_systemconfig.sCdrOsdCfg.time = iValue;
       cdr_osd_sw(0x03,iValue);
    }
    //速度水印
    if((strcmp("osd",pRoot)==0x00) && (strcmp("speed",pChildNode)==0x00))
    {
       iValue = atoi(pValue);
       g_cdr_systemconfig.sCdrOsdCfg.speed = iValue;
       cdr_osd_sw(0x04,iValue);
    }
    //发动机转速水印
    if((strcmp("osd",pRoot)==0x00) && (strcmp("engineSpeed",pChildNode)==0x00))
    {     
       iValue = atoi(pValue);
       g_cdr_systemconfig.sCdrOsdCfg.engineSpeed = iValue;
       cdr_osd_sw(0x05,iValue);//存在问题，关osd time时会关logo
    }
    //经纬度水印
    if((strcmp("osd",pRoot)==0x00) && (strcmp("position",pChildNode)==0x00))
    {
       iValue = atoi(pValue);
       g_cdr_systemconfig.sCdrOsdCfg.position = iValue;
       cdr_osd_sw(0x06,iValue);
    }
    //个性文字水印
    if((strcmp("osd",pRoot)==0x00) && (strcmp("personalizedSignature",pChildNode)==0x00))
    {     
       iValue = atoi(pValue);
       g_cdr_systemconfig.sCdrOsdCfg.personalizedSignature= iValue;
       cdr_osd_sw(0x07,iValue);//存在问题，关osd time时会关logo
    }

    if(strcmp("bluetooth",pRoot) == 0x00)
    {
	   iValue = atoi(pValue);
	   
#if 0	   
        if(iValue == 0x00)
        {
           if(g_cdr_systemconfig.bluetooth != 0x00){
            bt_close();
           }
        }
        if(iValue == 0x01)
        {
           if(g_cdr_systemconfig.bluetooth != 0x01){
            bt_open();
           }
        }
#else
		printf("bluetooth:%d\n",iValue);
		if(iValue == 0x00)
		{
			bt_close();
		}
		else if(iValue == 0x01)
		{
			bt_close();
			bt_open();
		}
#endif
        g_cdr_systemconfig.bluetooth = iValue;
    }
    if(strcmp("fmFrequency",pRoot) == 0x00)
    {
       iValue = atoi(pValue);
       cdr_set_qn8027_freq(iValue);
	   cdr_play_audio(CDR_AUDIO_FM,iValue);
	   
    }
    if(strcmp("telecontroller",pRoot)==0)
	{
	   iValue = atoi(pValue);
       cdr_lt8900_mode_ctrl(iValue);
   	   cdr_play_audio(CDR_AUDIO_LT8900PAIR,iValue);
    }
    if(strcmp("accelerationSensorSensitivity",pRoot) == 0x00)
	{
       iValue = atoi(pValue);
	   g_cdr_systemconfig.accelerationSensorSensitivity = iValue;
       cdr_bma250_mode_ctrl(iValue);
    }

	if(strcmp("photoWithVideo",pRoot) == 0x00)
	{
		g_cdr_systemconfig.photoWithVideo = atoi(pValue);
		cdr_play_audio(CDR_AUDIO_AssociatedVideo,g_cdr_systemconfig.photoWithVideo);
	}
    if(strcmp("net",pRoot)==0x00 && strcmp("apSsid",pChildNode)==0x00)
    {
     
      char cSedApSsidCmd[150] = {0};
      sprintf(cSedApSsidCmd,
        "sed -i '/EncrypType/ aiwpriv wlan0 set SSID=\"%s\"' /home/wifi_bin/init_wifi_7601.sh",
        pValue);
      cdr_system("sed -i '/SSID/d' /home/wifi_bin/init_wifi_7601.sh");
      cdr_system("sync");
      cdr_system(cSedApSsidCmd);
      cdr_system("sync");
           
    }
    if(strcmp("net",pRoot)==0x00 && strcmp("apPassword",pChildNode)==0x00)
    {
       char cSedApPasswdCmd[150] = {0};      
       sprintf(cSedApPasswdCmd,
        "sed -i '/SSID/ aiwpriv wlan0 set WPAPSK=\"%s\"' /home/wifi_bin/init_wifi_7601.sh",
        pValue);
       cdr_system("sed -i '/WPAPSK/d' /home/wifi_bin/init_wifi_7601.sh");
       cdr_system("sync");
       cdr_system(cSedApPasswdCmd);
       cdr_system("sync");
    }
	if(strcmp("net",pRoot)==0x00 && strcmp("Channel",pChildNode)==0x00)
    {     
      char cSedApSsidCmd[150] = {0};
	  int chn = atoi(pValue);
	  if(chn <0 || chn > 11)
	  	chn = 0;
	  //sed '2 a/xxx' b.txt
      sprintf(cSedApSsidCmd,"echo -e \"\nChannel=%d\" >> /etc/Wireless/RT2870AP/RT2870AP.dat",chn);
      cdr_system("sed -i '/Channel=/d' /etc/Wireless/RT2870AP/RT2870AP.dat");	  
      cdr_system("sync");
      cdr_system(cSedApSsidCmd);
	  cdr_system("sed -i '/^$/d' /etc/Wireless/RT2870AP/RT2870AP.dat");	  
      cdr_system("sync");   

	  sleep(1);

	  cdr_system("reboot");
    }
    return 0;
}


int cdr_syscfg_set_pro(int clientindex)
{
    int childflag = 0,i=0,level = 0;
    char chBody[200]  = {0};
    char cfgName[40]  = {0};
    char rootNode[40] = {0};
    char childNode[40] = {0};
    char value[40] = {0};

	int msgLen = g_appclients[clientindex].proRecvBuffer.proheader.MsgLenth;
	memset(chBody,0x00,sizeof(chBody));
	memcpy(&chBody,&g_appclients[clientindex].proRecvBuffer.MsgBody,msgLen);
	
	//Get number of char '.'
	for(i=0;i<msgLen;i++)
	{
		if(chBody[i] == '.')
			level++;
		if(chBody[i] == '\"')
			break;
	}

	//printf("level :%d\r\n",level);
	
	//foramt:"cdrSystemCfg.osd.time="1""
	if(level == 2)
	{
		if(4 == sscanf(chBody,"%[^.].%[^.].%[^=]=\"%[^\"]\"",cfgName,rootNode,childNode,value))
		{
			childflag = 1;
		}
		else
		{
			return -1;
		}
	}

	//format : "cdrSystemCfg.bootVoice="0""
	if(level == 1)
	{
		if(3 == sscanf(chBody,"%[^.].%[^=]=\"%[^\"]\"",cfgName,rootNode,value))
		{
			childflag = 0;
		}
		else
		{
			return -1;
		}
	}
		
	if(strcmp(cfgName, "cdrSystemCfg") != 0)
	{
		printf("cfgName:%s\r\n",cfgName);
		return -1;
	}

    cdr_syscfg_set_pro_impl(rootNode,childNode,value,childflag);   

	return save_cfg_to_xml(rootNode,childNode,value,childflag);
	
}

int cdr_system(const char * cmd) 
{ 
    FILE * fp; 
    int res; char buf[1024]; 
    if (cmd == NULL) 
    { 
        printf("my_system cmd is NULL!\n");
        return -1;
    } 
    if ((fp = popen(cmd, "r") ) == NULL) 
    { 
        perror("popen");
        printf("popen error: %s/n", strerror(errno)); return -1; 
    } 
    else
    {
         while(fgets(buf, sizeof(buf), fp)) 
        { 
            printf("%s", buf); 
        } 
        if ( (res = pclose(fp)) == -1) 
        { 
            printf("close popen file pointer fp error!\n"); return res;
        } 
        else if (res == 0) 
        {
            return res;
        } 
        else 
        { 
            printf("popen res is :%d\n", res); return res; 
        } 
    }
 }




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


