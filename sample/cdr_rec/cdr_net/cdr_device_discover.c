
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h> 

#include <ifaddrs.h>  
#include <netinet/in.h>   
#include <sys/vfs.h>  

#include "cdr_device_discover.h"

#define HIPC_RECV_PORT 12330
#define CDR_WIFI_INTERFACE "wlan0"

struct sockaddr_in PCUDP_HiAddr;
int PcCmdSocket = -1;
pthread_t PCUDPCmdRevcThreadPid;


typedef enum
{			   
	UNDEFINE,
	CMD_BROADCAST = 0xFF,	      
}CCmd;

typedef enum					 
{
	NoErr=0,
	CMDErr,				   
	BagLengthErr,		   
	RangeErr,			    
	}eErrCode;

typedef enum
{
	IF_UART,	   
	IF_NET,	       
	IF_UNDEFINE,   
}IFType;



#define PACKED __attribute__((packed, aligned(1)))

#define MSGBODY_MAX_LEN 200
typedef struct {
	unsigned char    ucHeadFlag[4];
	unsigned char    ucCmdID;
	unsigned short   usMsgSN;
	unsigned char    ucVersionFlag[2];
	unsigned char    ucToken[32];
	unsigned short	 usMsgLenth;
}PACKED sProtHeaderPack;

typedef struct {
	sProtHeaderPack sProtHeader;
    //unsigned char   *pucMsgBody;
    unsigned char   ucMsgBody[MSGBODY_MAX_LEN];
    unsigned short  usVerifyCode;

}PACKED sCDR_ProtPack;


struct sockaddr_in boastfrom;  
int HiPCRevcPort;
char g_addressBuffer[INET_ADDRSTRLEN];  //16 存储本设备端的ip

unsigned char * const pucDeviceReturnHeader = "EYEC";

//test
void * PCUDPCmdRevcThread(void *pArgs);
void ProtoRetuBagHandle(void);
void PCCmdUdpSockInit(int HiPCRevcPort);

/*
pc端的udp 命令接收 处理
*/
void cdr_device_discover_init(void)
{
    HiPCRevcPort = HIPC_RECV_PORT;
    
    PCCmdUdpSockInit(HiPCRevcPort);	

    pthread_create(&PCUDPCmdRevcThreadPid, NULL, (void*)PCUDPCmdRevcThread,NULL);

    //pthread_join(PCUDPCmdRevcThreadPid, 0);
    pthread_detach(PCUDPCmdRevcThreadPid);        
}


/*
udp 通信，海思板作为服务端接收pc 端发过来的数据
*/
void PCCmdUdpSockInit(int HiPCRevcPort)
{
    //广播部分 绑定地址  
	struct sockaddr_in addrto;  
	bzero(&addrto, sizeof(struct sockaddr_in));	
	addrto.sin_family = AF_INET;  
	addrto.sin_addr.s_addr = htonl(INADDR_ANY);	
	addrto.sin_port = htons(HiPCRevcPort);  

	// 广播地址	
	bzero(&boastfrom, sizeof(struct sockaddr_in));  
	boastfrom.sin_family = AF_INET;  
	boastfrom.sin_addr.s_addr = htonl(INADDR_ANY);  
	boastfrom.sin_port = htons(HiPCRevcPort);  

	if ((PcCmdSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)	
	{	 
        printf(" socket error [%s,%d]\r\n",__FUNCTION__,__LINE__);
	   return ;  
	}	 

	const int opt = 1;  
	int nb = setsockopt(PcCmdSocket, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));//设置该套接字为广播类型， 	
	if(nb == -1)  
	{  
	   printf("set socket opt error [%s,%d]\r\n",__FUNCTION__,__LINE__);
	   return ;  
	}  

	if(bind(PcCmdSocket,(struct sockaddr *)&(addrto), sizeof(struct sockaddr_in)) == -1)   
	{	 
        printf(" bind error [%s,%d]\r\n",__FUNCTION__,__LINE__);
	   return ;  
	}  	
}



/*
*接收pc/phone 端发送过来的udp数据
*/
void * PCUDPCmdRevcThread(void *pArgs)
{
	unsigned char ucPCRevcBuff[300] = {0};//245
	int iRecvCount = 0;	
	
	int len = sizeof(struct sockaddr_in);
	CCmd ucInputCMD = 0x00;      //CmdID
	
	unsigned char ucHeaderBuffer[5]={0};

	while(1)
	{	  
		usleep(50000);	  
		iRecvCount=recvfrom(PcCmdSocket, ucPCRevcBuff,sizeof(ucPCRevcBuff), MSG_NOSIGNAL, (struct sockaddr*)&boastfrom,(socklen_t*)&len);  
		if (iRecvCount > 0) 
		{  

			printf("%s iRecvCount:%d\r\n",__FUNCTION__,iRecvCount);
			memcpy(ucHeaderBuffer,ucPCRevcBuff,4);
			ucHeaderBuffer[4]='\0';        
			if(strcmp(pucDeviceReturnHeader,ucHeaderBuffer)!=0)
			{
				//printf("ucHeaderBuffer:%s\r\n",ucHeaderBuffer);
				continue;
			}
			ucInputCMD = ucPCRevcBuff[4];
			switch(ucInputCMD)
			{		
				case CMD_BROADCAST:             
					ProtoRetuBagHandle();//当前是广播命令，则返回当前的ip设备名信息
					break;
				default: 
					break;		  
			}	
			iRecvCount = 0;
			memset(&ucPCRevcBuff,0,sizeof(ucPCRevcBuff));
		}	  
	}
}


int cdr_get_host_mac(char *pMac)
{ 
	struct	 ifreq	 ifreq; 
	int   sock; 

	if(pMac == NULL)
		return -1;
	
	if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
	{ 
		perror( "socket "); 
		return	 -1; 
	} 
	strcpy(ifreq.ifr_name,CDR_WIFI_INTERFACE); 
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
	{ 
		perror( "ioctl "); 
		return	 -1; 
	} 
	sprintf(pMac,"%02X%02X%02X%02X%02X%02X", 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[0], 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[1], 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[2], 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[3], 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[4], 
			(unsigned	char)ifreq.ifr_hwaddr.sa_data[5]); 
	return	 0; 
}

/*
*获取本机的ip
*/
void cdr_get_host_ip()
{
    struct ifaddrs * ifAddrStruct = NULL;  
    void * tmpAddrPtr = NULL;   
    getifaddrs(&ifAddrStruct);  
  
    while(ifAddrStruct!=NULL)   
    {  
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)  
        {   // check it is IP4  
            // is a valid IP4 Address  
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;  
            
            inet_ntop(AF_INET, tmpAddrPtr, g_addressBuffer, INET_ADDRSTRLEN);  
        }  
        else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)  
        {   // check it is IP6  
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;  
            char addressBuffer[INET6_ADDRSTRLEN];  
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);  
            //printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);   
        }   
        ifAddrStruct = ifAddrStruct->ifa_next;  
    } 
}

//----------------------------------------------------------------------------
//**Description: 回复包处理函数
//**input:
//**output:
//**return:
//----------------------------------------------------------------------------

void ProtoRetuBagHandle()
{
    unsigned char ucReturnBuffer[300] = {0},chMsgBody[50],chMac[13];
	int len = 0;
    //int i ;
    int addr_len = sizeof(struct sockaddr_in);

    sProtHeaderPack sProtoclHeaderMumber;
    sCDR_ProtPack sProtPackMumber;

    cdr_get_host_ip();
    cdr_get_host_mac(chMac);
	
    chMac[12] = 0x00;
    sprintf(chMsgBody,"%s,%s",g_addressBuffer,chMac);

    //1,header	
    memcpy(sProtoclHeaderMumber.ucHeadFlag,"EYES",4);
    sProtoclHeaderMumber.ucCmdID= 0xff;
    sProtoclHeaderMumber.usMsgSN = 0x0001;
    sProtoclHeaderMumber.ucVersionFlag[0] = 0x00;
    sProtoclHeaderMumber.ucVersionFlag[1] = 0x01;
    memset(sProtoclHeaderMumber.ucToken,0,sizeof(sProtoclHeaderMumber.ucToken));
    sProtoclHeaderMumber.usMsgLenth = strlen(chMsgBody);

    //header
    sProtoclHeaderMumber.usMsgLenth = htons(sProtoclHeaderMumber.usMsgLenth);
    memcpy(&sProtPackMumber.sProtHeader,&sProtoclHeaderMumber,sizeof(sProtoclHeaderMumber));

    //body
    memset(sProtPackMumber.ucMsgBody,0xff,sizeof(sProtPackMumber.ucMsgBody));
    //memcpy(&sProtPackMumber.ucMsgBody,g_addressBuffer,strlen(g_addressBuffer));   
	memcpy(&sProtPackMumber.ucMsgBody,chMsgBody,strlen(chMsgBody));  
    //printf("sProtPackMumber.ucMsgBody %s \n",sProtPackMumber.ucMsgBody);   
    
    //3crc     
    len = sizeof(sProtoclHeaderMumber)+strlen(chMsgBody);
    memcpy(ucReturnBuffer,(unsigned char*)&sProtPackMumber, len);    
    //for( i =0;i<len;i++)  printf(" %02x ",ucReturnBuffer[i]);  printf("\n");

    sProtPackMumber.usVerifyCode = get_crc16((unsigned char*)&sProtPackMumber, len);
    len+=2;
    ucReturnBuffer[len-2] = (sProtPackMumber.usVerifyCode)>>8;
    ucReturnBuffer[len-1] = (unsigned char)((sProtPackMumber.usVerifyCode)&0xff);

    //sProtPackMumber.usVerifyCode = htons(sProtPackMumber.usVerifyCode);
    //sendto(PcCmdSocket,(unsigned char*)&sProtPackMumber,len,0, (struct sockaddr*)&boastfrom,addr_len); 	
    sendto(PcCmdSocket,ucReturnBuffer,len,0, (struct sockaddr*)&boastfrom,addr_len); 	
	
}


unsigned short get_crc16( unsigned char* buffer,int len)
{
	unsigned short crc = 0x00;
	unsigned short current = 0x00;
	int i = 0;
    int j;
	unsigned char ch = 0x00;
	for(i=0;i<len;i++)
	{
		ch = buffer[i];
		current = (ch&0x000000FF) << 8;
		for(j=0;j<8;j++)
		{
			if((crc ^ current)&0x8000){
				crc = (crc << 1) ^ 0x1021;
			}else{
				crc <<=1;
			}
			crc &=0x0000FFFF;
			current <<=1;		
		}
	}	
	return crc;
}





