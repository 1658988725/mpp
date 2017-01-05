/************************************************************************	
** Filename: 	cdr_comm.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <signal.h>
#include "cdr_comm.h"

#define SEND_FIFO "/tmp/myfifo"    //send fifo
#define REVC_FIFO "/tmp/myfifo1"   //revc fifo

static unsigned char g_ucExitFlag = 0;

char g_pCdrStartDateTimeBuf[120] = {0};


int recspslen = 0;
int recppslen = 0;

unsigned char recspsdata[64];
unsigned char recppsdata[64];


void cdr_log_get_time(unsigned char * ucTimeTemp)
{   
    if(ucTimeTemp == NULL)
    {
      printf("[%s %d] ucTimeTemp is null\n",__FUNCTION__,__LINE__);
      return;
    }
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(ucTimeTemp, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
}

void cdr_get_curr_time(unsigned char * ucTimeTemp)
{   
    #if(0)
    if(ucTimeTemp == NULL)
    {
      printf("[%s %d] ucTimeTemp is null\n",__FUNCTION__,__LINE__);
      return;
    }
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(ucTimeTemp, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
    #else
    cdr_log_get_time(ucTimeTemp);
    #endif
}


int set_cdr_start_time()
{
    char pDateTimeBuf[120] = {0};
    
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(pDateTimeBuf, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

    memcpy(g_pCdrStartDateTimeBuf,pDateTimeBuf,strlen(pDateTimeBuf)+1);

    set_cdr_start_sec();
        
    return 0;
}

static int cdr_system_self(const char * cmd) 
{ 
    FILE * fp; 
    int res = 0; 
    char buf[1024] = {0}; 
    
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

static void timer(int sig)  
{  
    if(SIGALRM == sig)  
    {  
       g_ucExitFlag = 1;   
    }  
    return ;  
} 

//read system process execuce 's result
static int cdr_revc_systm_result()
{
    int fd = 0;
    int iReadCount = 0;
    int iResult = 0;
    //unsigned char ucMsg[20] = {0};
    char ucReadBufArr[200] = {0};

    unlink(REVC_FIFO);
    if(access(REVC_FIFO, F_OK) == -1){
       if((mkfifo(REVC_FIFO,O_CREAT|O_EXCL)<0)&&(errno!=EEXIST)){
            printf("[%s %d]cannot create fifoserver\n",__FUNCTION__,__LINE__);
            return -1;
       }
    }
    
    fd = open(REVC_FIFO,O_RDONLY|O_NONBLOCK,0);
    if(fd == -1)
    {
      perror("open");
      exit(1);
    }
    //开启定时器；若1分钟还没有收到返回数据则自动失败不继续接收
    signal(SIGALRM, timer); 
    g_ucExitFlag = 0;    
    alarm(60*1);     

    while(1)
    {
       memset(ucReadBufArr,0,sizeof(ucReadBufArr));
       iReadCount = read(fd,ucReadBufArr,100);
       if(iReadCount ==-1){
         printf("[%s %d]read from system process\n",__FUNCTION__,__LINE__);
         if(errno==EAGAIN) printf("no data yet\n");
       }
       if(iReadCount>0){
          //printf("read %s from  iReadCount %02x \n",ucReadBufArr,iReadCount);          
          iResult = atoi(ucReadBufArr);
          //printf("iResult:%d\n",iResult);
          break;
       }
       if(g_ucExitFlag)break;       
       sleep(1);
    }
    return iResult;
}

static int cdr_send_msg_to_system(char *cmd)
{
    int fd;
    char w_buf[200] = {0};
    int nwrite;
    int iResult = 0;
    
    if(access(SEND_FIFO, F_OK) == -1){
       printf("当前的fifo不存在\n");
       return -1;
    }           
    fd=open(SEND_FIFO,O_WRONLY|O_NONBLOCK,0);
    if(fd==-1){
       if(errno==ENXIO){
        printf("open error;no reading process\n");
        return -1;
       }
    }

    memcpy(w_buf,cmd,strlen(cmd)+1);    
    if((nwrite=write(fd,w_buf,strlen(w_buf)+1))==-1)
    {
      if(errno==EAGAIN){
        printf("The FIFO has not been read yet. Please try later\n");
        return -1;
      }
    }else{ 
      //printf("%s\n",w_buf);//发送buf完成
      iResult = cdr_revc_systm_result(); //接收另一个进程 返回的执行结果
      printf("[%s %d] get system process exece result %d\n",__FUNCTION__,__LINE__,iResult);
    }    
    return iResult;
}

int cdr_system_new(const char * cmd) 
{ 
    int iResult = 0;
    //FILE * fp; 
    //int res = 0; 
    //char buf[1024] = {0}; 
    
    iResult = cdr_system_self(cmd);
    if(iResult == 0)
    {
      return 0;
    }
    
    if (cmd == NULL) 
    { 
        printf("my_system cmd is NULL!\n");
        return -1;
    }     
    iResult = cdr_send_msg_to_system(cmd);

    if(iResult == -1)    
    {
      iResult = cdr_system_self(cmd);
    }    
    return iResult;
 }

#if(0)
void main(void)
{
  const unsigned char *pCmd="ls -l";  
  //const unsigned char *pCmd="ls -l";    
  cdr_system(pCmd);

  while(1);
}
#endif

//把时间字符串转为秒.
time_t _strtotime(char *pstr)
{
	if(pstr == NULL)
		return 0;
	
	//printf("%s\r\n",pstr);
	int y,m,d,h,M,s;
	sscanf(pstr,"%04d%02d%02d%02d%02d%02d",&y,&m,&d,&h,&M,&s);
	
	struct tm t;
	time_t t_of_day;//long int t_of_day
	t.tm_year=y-1900;
	t.tm_mon=m-1;
	t.tm_mday=d;
	t.tm_hour=h;
	t.tm_min=M;
	t.tm_sec=s;
	t.tm_isdst=0;
	t_of_day=mktime(&t);
	//printf("%s:%ld\r\n",pstr,t_of_day);
	return t_of_day;		
}

