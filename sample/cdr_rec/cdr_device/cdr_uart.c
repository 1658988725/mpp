/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>
*************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sample_comm.h"
#include "cdr_uart.h"
#include "cdr_queue.h"

static void init_uart_handle_thread(void) ;

#define UART_DEVICE "/dev/ttyAMA2"

#define FALSE  -1
#define TRUE    0

int g_uart_thread_flag = 0;

int speed_arr[] = 
{
 B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
 B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, 
};
int name_arr[] = 
{
 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 
 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 
};

unsigned char g_ucGpsDataBuff[300] = {0};
static cdr_uart_callback g_uart_fun_callbak;

int g_uart2fd = -1;

/**
*@brief 设置串口通信速率
*@param fd 类型 int 打开串口的文件句柄
*@param speed 类型 int 串口速度
*@return void
*/
static void set_speed(int fd, int speed){
  int i; 
  int status; 
  struct termios Opt;
  tcgetattr(fd, &Opt);   
  for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
  { 
    if (speed == name_arr[i]) { 
      tcflush(fd, TCIOFLUSH); 
      cfsetispeed(&Opt, speed_arr[i]); 
      cfsetospeed(&Opt, speed_arr[i]); 
      status = tcsetattr(fd, TCSANOW, &Opt); 
      if (status != 0) { 
        perror("tcsetattr fd1\n"); 
        return; 
      } 
      tcflush(fd,TCIOFLUSH); 
    } 
  }
}


/**
*@brief 设置串口数据位，停止位和效验位
*@param fd 类型 int 打开的串口文件句柄
*@param databits 类型 int 数据位 取值 为 7 或者8
*@param stopbits 类型 int 停止位 取值为 1 或者2
*@param parity 类型 int 效验类型 取值为N,E,O,,S
*/
static int set_Parity(int fd,int databits,int stopbits,int parity)  
{   
    struct termios options;   
    if  ( tcgetattr( fd,&options)  !=  0) {   
        perror("SetupSerial 1");       
        return(FALSE);    
    }  
    options.c_cflag &= ~CSIZE;   

    options.c_cflag |= (CLOCAL | CREAD); //一般必设置的标志
    
    switch (databits) /*设置数据位数*/  
    {     
    case 7:       
        options.c_cflag |= CS7;   
        break;  
    case 8:       
        options.c_cflag |= CS8;  
        break;     
    default:      
        fprintf(stderr,"Unsupported data size\n"); return (FALSE);    
    }  
    switch (parity)   
    {     
        case 'n':  
        case 'N':      
            options.c_cflag &= ~PARENB;    /* Clear parity enable */  
            options.c_iflag &= ~INPCK;     /* Enable parity checking */   
            break;    
        case 'o':     
        case 'O':       
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/    
            options.c_iflag |= INPCK;             /* Disnable parity checking */   
            break;    
        case 'e':    
        case 'E':     
            options.c_cflag |= PARENB;      /* Enable parity */      
            options.c_cflag &= ~PARODD;     /* 转换为偶效验*/       
            options.c_iflag |= INPCK;       /* Disnable parity checking */  
            break;  
        case 'S':   
        case 's':  /*as no parity*/     
            options.c_cflag &= ~PARENB;  
            options.c_cflag &= ~CSTOPB;break;    
        default:     
            fprintf(stderr,"Unsupported parity\n");      
            return (FALSE);    
        }    
    /* 设置停止位*/    
    switch (stopbits)  
    {     
        case 1:      
            options.c_cflag &= ~CSTOPB;    
            break;    
        case 2:      
            options.c_cflag |= CSTOPB;    
           break;  
        default:      
             fprintf(stderr,"Unsupported stop bits\n");    
             return (FALSE);   
    }   
    /* Set input parity option */   
    if (parity != 'n')     
        options.c_iflag |= INPCK;   

    options.c_cflag |= (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    options.c_oflag &= ~OPOST;
    options.c_oflag &= ~(ONLCR | OCRNL); 

    options.c_iflag &= ~(ICRNL | INLCR);
    options.c_iflag &= ~(IXON | IXOFF | IXANY); 

    tcflush(fd,TCIFLUSH); 
    
    options.c_cc[VTIME] = 150; /* 设置超时15 seconds*/     
    options.c_cc[VMIN] = 0;    /* Update the options and do it NOW */  
    if (tcsetattr(fd,TCSANOW,&options) != 0)     
    {   
        perror("SetupSerial 3");     
        return (FALSE);    
    }    
    return (TRUE); 
}

#if 0
static int cdr_open_uart(char *name)
{
    int fd = 0;
    fd = open(name, O_RDWR);
    if(fd < 0)
    {
        perror(name);
    	return -1;
    }

    fcntl(fd,F_SETFL,O_NONBLOCK);//以非阻塞 模式打开 

    set_speed(fd,9600);		
    if (set_Parity(fd,8,1,'N') == FALSE)
    {
      printf("Set Parity Error\n");
      return -1;
    }
    return fd;
}
#endif


int cdr_uart_init_factory(void)
{    
    return g_uart2fd;
}

#define  UART_DEBUG  0
/*
0，环型队列的实现
1, 添加read 的数据到队列；
2，从队列中取一个gprm包  从环型队列中取一个Bag 2.1从一固定字符串中取指定gprm包，头与尾的校验
*/
int uart_get_dst_bag(char *pSrcBuf,char *pDstBag)
{
    int i = 0;
    int m = 0; 
    int n = 0;
    int res = 0;     
    static int j = 0;
    static char pDstBagTemp[200] = {0};
    static char cStartFlag = 0;
    static char cStopFlag = 0;

    int index1 = 0,index2 = 0;
   
    assert((pSrcBuf!=NULL)&&(pDstBag!=NULL));

#if UART_DEBUG
    printf("stUartQueue.usBufSize:%d\n",stUartQueue.usBufSize);
    printf("\r\n");
    printf("pSrcBuf:\n%s \nstUartQueue.usHead:%d stUartQueue.usTail:%d\n",pSrcBuf,stUartQueue.usHead,stUartQueue.usTail);     
#endif    

    /*循环遍历整个队列 RMC*/
    for(i=stUartQueue.usHead;;i++,n++)
    {

     if(n>stUartQueue.usBufSize)
     {        
        return -1;
     }
     
     if(QueueCheckEmpty(&stUartQueue)==1)
     {
        return -1;
     }
     if(i == stUartQueue.usBufSize) i = 0;
     if(pSrcBuf[i] == '$')
     {
        i++;
        if(i == stUartQueue.usBufSize) i = 0;
        if((pSrcBuf[i] == 'G')||(pSrcBuf[i] == 'B')){/*BD GN GP 可优化*/
            i++;
            if(i == stUartQueue.usBufSize) i = 0;
            //if(pSrcBuf[i] == 'P'){//gps
            //if((pSrcBuf[i] == 'N')||(pSrcBuf[i] == 'P')){//北斗双模
            if((pSrcBuf[i] == 'N')||(pSrcBuf[i] == 'P')||(pSrcBuf[i] == 'D')){
               i++;
               if(i == stUartQueue.usBufSize) i = 0;
               if(pSrcBuf[i] == 'R'){
                  i++;
                  if(i == stUartQueue.usBufSize) i = 0;
                  if(pSrcBuf[i] == 'M'){ 
                     i++;
                     if(i == stUartQueue.usBufSize) i = 0;
                     if(pSrcBuf[i] == 'C')
                     {  
                       //get the head
                       index1 = i;

                       if(i>=5)stUartQueue.usHead = i - 5;//头指针移动
                       else stUartQueue.usHead = (i+stUartQueue.usBufSize) - 5;//头指针移动
                       
                       j = 0;
                       for(m=0;m<6;m++)
                       {
                         res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);
                         if(res == 0x01)
                         {
                            j++;
                         }
                       }     
                       cStartFlag = 0x01;
                       continue;
                     }
                   }
                 }
             }
         }

		
     }     
     if((cStartFlag == 0x01)&&(pSrcBuf[i] != '\r'))
     {
       //get the body
       res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);       
       if(res == 0x01)
       {
         j++;
       }
     }
     if((pSrcBuf[i] == '\r')&&(cStartFlag == 0x01))
     {    
        //get the end
        index2 = i;
        QueueGetData(&stUartQueue,&pDstBagTemp[j]);        
        i++;
        j++;
        if(i == stUartQueue.usBufSize) i = 0;
        
        if(pSrcBuf[i] == '\n')
        {
           QueueGetData(&stUartQueue,&pDstBagTemp[j]);
           j++;
           pDstBagTemp[j] = '\0';
           cStopFlag = 0x01;
           break;
        }       
     }
    }
    /*截取完成*/
    if(cStopFlag == 0x01)
    {
      cStartFlag = 0;
      cStopFlag = 0;
      j = 0;
#if UART_DEBUG    
      //printf("pDstBagTemp: %d\n%s\n",strlen(pDstBagTemp),pDstBagTemp);
#endif
      memcpy(pDstBag,pDstBagTemp,strlen(pDstBagTemp));//获取一个指定的包
      memset(pDstBagTemp,0,sizeof(pDstBagTemp));
      return 0;
    }else{
      //printf("data is invalid\n");
      return -1;
    }  
}

#if(0)
int uart_get_dst_bag(char *pSrcBuf,char *pDstBag)
{
    int i = 0;
    int m = 0; 
    int res = 0;     
    static int j = 0;
    static char pDstBagTemp[200] = {0};
    static char cStartFlag = 0;
    static char cStopFlag = 0;
   
    assert((pSrcBuf!=NULL)&&(pDstBag!=NULL));

    printf("stUartQueue.usBufSize:%d\n",stUartQueue.usBufSize);
    printf("pSrcBuf:\n%s \nstUartQueue.usHead:%d stUartQueue.usTail:%d\n",pSrcBuf,stUartQueue.usHead,stUartQueue.usTail);     

    for(i=stUartQueue.usHead;;i++)
    {
     if(QueueCheckEmpty(&stUartQueue)==1)
     {
        printf("QueueCheckEmpty\n");
        break;         
     }
     if(i == stUartQueue.usBufSize) i = 0;
     if(pSrcBuf[i] == '$')
     {
        i++;
        if(i == stUartQueue.usBufSize) i = 0;
        if(pSrcBuf[i] == 'G'){
            i++;
            if(i == stUartQueue.usBufSize) i = 0;
            if(pSrcBuf[i] == 'P'){
               i++;
               if(i == stUartQueue.usBufSize) i = 0;
               if(pSrcBuf[i] == 'R'){
                  i++;
                  if(i == stUartQueue.usBufSize) i = 0;
                  if(pSrcBuf[i] == 'M'){ 
                     i++;
                     if(i == stUartQueue.usBufSize) i = 0;
                     if(pSrcBuf[i] == 'C')
                     {  
                       //get the head
                       printf("get the head\n");
                       j = 0;
                       for(m=0;m<6;m++)
                       {
                         res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);
                         if(res == 0x01)
                         {
                            //printf("%c\n",pDstBagTemp[j]);
                            j++;
                         }
                       }     
                       cStartFlag = 0x01;
                       continue;
                     }
                   }
                 }
             }
         }
     }     
     if((cStartFlag == 0x01)&&(pSrcBuf[i] != '\r'))
     {
       //get the body
       //printf("get the body\n");
       res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);       
       if(res == 0x01)
       {
         j++;
       }
     }
     if((pSrcBuf[i] == '\r')&&(cStartFlag == 0x01))
     {    
        //get the end
        printf("get the end\n");
        QueueGetData(&stUartQueue,&pDstBagTemp[j]);        
        i++;
        j++;
        if(i == stUartQueue.usBufSize) i = 0;
        
        if(pSrcBuf[i] == '\n')
        {
           printf("pDstBagTemp[j] = '\0'\n");
           QueueGetData(&stUartQueue,&pDstBagTemp[j]);
           j++;
           pDstBagTemp[j] = '\0';
           cStopFlag = 0x01;
           break;
         }       
     }
    }

    if(cStopFlag == 0x01)
    {
      printf("cStopFlag == 0x01\n");
      cStartFlag = 0;
      cStopFlag = 0;
      j = 0;
      memcpy(pDstBag,pDstBagTemp,strlen(pDstBagTemp));//获取一个指定的包
      memset(pDstBagTemp,0,sizeof(pDstBagTemp));
      return 0;
    }else{
      printf("data is invalid\n");
      return -1;
    }  
}
#endif


static void cdr_get_uart_data_thread(void * pArgs)
{    
    int res = 0;
    int i = 0;
    //int ii = 0;    
    //int count = 0;    
    unsigned char buf[1024] = {0};
    //char cBuffArr[400] = {0};        
    UartRecvQueueInit(&stUartQueue);    
    cdr_uart_callback uartfcallbak = g_uart_fun_callbak;

	fd_set rd;
	int nread = 0;


#if 1//实时数据接收  

	while(1) 
	{
		FD_ZERO(&rd);
		FD_SET(g_uart2fd, &rd);
		while (FD_ISSET(g_uart2fd, &rd))
		{
			if (select(g_uart2fd + 1, &rd, NULL, NULL, NULL) < 0)
			{
				perror("select error\n");
			}
			else
			{
				//9600波特率，延时10ms基本可以保证数据一次性收取完整.
				usleep(10000);
				while ((nread = read(g_uart2fd, buf + res, 100)) > 0)
				{
					res += nread;
					usleep(10000);
				}
				for(i=0;i<res;i++)
				{
					QueueAddData(&stUartQueue,buf[i]);
				}   

				if(uartfcallbak)
				{
					uartfcallbak(buf);				
				}  
				res = 0;
				memset(buf, 0x00, sizeof(buf));
			}
		}
	}
#else//文件读取

    FILE * fdTest = fopen("/home/gps_20160817090212_2406.log","r");
    if(fdTest == NULL){
       printf("not find /home/gps_20160817090212_2406.log");
       close(fd);
       return ;
    }
    while(g_uart_thread_flag) 
	{
       usleep(1000000);   //sleep(1);		      
       uartfcallbak = g_uart_fun_callbak;
       if(feof(fdTest))
       {
          fseek(fdTest,0L,SEEK_SET);//cycle read file
          continue;
       }       
       memset(buf,0,sizeof(buf));
       res = fread(buf,1,74, fdTest);
       //res = fgets(buf,sizeof(buf),fdTest);
       if(res!= 0 && res!= -1)
	   {
           for(i=0;i<res;i++)
           {
             QueueAddData(&stUartQueue,buf[i]);
           }   
           if(uartfcallbak)
           {
             uartfcallbak(buf);				
           }   
	   }     
	}
    close(fdTest);
#endif

    close(g_uart2fd);
    return 0;	
}


static void init_uart_handle_thread(void) 
{ 	
	pthread_t tfid;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, cdr_get_uart_data_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }
    
    pthread_detach(tfid);
}

int cdr_uart_deinit(void)
{    
	g_uart_thread_flag = 0;
	return 1;
}


static int cdr_uart_callback_imp(unsigned char *ucValue)
{  
  int ret = 0;
    
  
  return ret;
}

void cdr_uart_setevent_callbak(cdr_uart_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n");
		return;
	}
    
	g_uart_fun_callbak = callback;

    if(g_uart_fun_callbak  != NULL)	
    {
		printf("%s register g_uart_fun_callbak . \r\n",__FUNCTION__);
    }

}


void cdr_uart2_printf(char *ch)
{
	char chUart[200];
	memset(chUart,0x00,200);
	sprintf(chUart,"%s",ch);
	write(g_uart2fd,chUart,strlen(chUart)+1);
}

static int  int_uart()
{
    int ret = 0;
    cdr_system("himm  0x200F00D0  0x03");//uart tx 
    cdr_system("himm  0x200f00CC  0x03");//uart rx
    usleep(10000);    

	g_uart2fd = open(UART_DEVICE, O_RDWR);
    if(g_uart2fd < 0)
    {
        perror(UART_DEVICE);
	    return -1;
    }

	fcntl(g_uart2fd,F_SETFL,O_NONBLOCK);//O_NONBLOCK :非阻塞  0 :阻塞 

    set_speed(g_uart2fd,9600);		
    if (set_Parity(g_uart2fd,8,1,'N') == FALSE)
    {
		printf("Set Parity Error\n");
		return -1;
	}
	
    return ret;
}

int cdr_uart_init(void)
{    
    int_uart();
    g_uart_thread_flag = 1;	
	init_uart_handle_thread();
	return 1;
}

