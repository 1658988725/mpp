/************************************************************************	
** Filename: 	cdr_device.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2011 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <memory.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <sys/time.h>

#include "cdr_bma250e.h"
#include "cdr_lt8900.h"

#include "cdr_app_service.h"
#include "cdr_XmlLog.h"

#include "cdr_gps_data_analyze.h"

#include "cdr_device.h"
#include "cdr_writemov.h"
#include "cdr_bt.h"

#include "cdr_led.h"
#include "cdr_key_check.h"
#include "cdr_gp.h"
#include "cdr_fm.h"
#include "cdr_uart.h"
#include "cdr_queue.h"
#include "cdr_config.h"



#define BUF_SIZE 1024

char g_cdr_device_app_stop_flag = CDR_POWER_ON;
GPS_INFO g_GpsInfo;

#define SINGLE_PRESS 0x01
#define LONG_PRESS   0x02
#define DOUBLE_PRESS 0x03
#define HOLD_KEY     0x04


unsigned char ucRedFlag = 0;

#define LED_CTRL_SW 1

static unsigned char ucCalibTimeFlag = 0x00;

int cdr_start_factest(void);

static int iForceRecordTime = 0;
static unsigned char ucForceRecordTimeOutFlag = 0;
static unsigned char ucForceRecordFullName[256] = {0};
static unsigned char g_ucStartForcedRecordFlag = 0;
static unsigned char g_ucStopForcedRecordFlag = 0;

static int iStartForcedRecordSec = 0;
static int iForcedRecordTotalSec = 0;

/*
get forced record jpg full name
pDst:输出参数 ，存放强制录制对应图片的全名
*/
int GetForcedRecordFullName(char *pDst)
{
  if(pDst==NULL)return -1;
  
  memcpy(pDst,ucForceRecordFullName,strlen(ucForceRecordFullName));
  
  return 0;
}


void uninit_time()  
{  
    struct itimerval value;  
    value.it_value.tv_sec = 0;  
    value.it_value.tv_usec = 0;  
    value.it_interval = value.it_value;  
    setitimer(ITIMER_REAL, &value, NULL);  
}

void ForcedRecordSigSubroutine(int signo)
{
  char cFullJpgName[256] = {0};
  char cFullJpgNameHead[256] = {0};

  char cFullJpgNameHead1[256] = {0};
  char cFullJpgNameHead2[256] = {0};
  char cFullJpgNameHead3[20] = {0};
  char *pcFullJpgNameHead2;
 
  switch (signo)
  {
   case SIGALRM:
       //iForceRecordTime++;//强制录制的时间
       ucForceRecordTimeOutFlag++;
       //printf("ucForceRecordTimeOutFlag:%d\n",ucForceRecordTimeOutFlag);
       //if(ucForceRecordTimeOutFlag>0x05)//当超过指定时间还没有收到0x04的按键值过来时
       if(ucForceRecordTimeOutFlag>0x05)//当超过指定时间还没有收到0x04的按键值过来时
       {
        GetFullJpgName(cFullJpgName);
        sscanf(cFullJpgName,"%[^.]",cFullJpgNameHead);
        //printf("cFullJpgNameHead:%s\n",cFullJpgNameHead); 

        //sscanf(cFullJpgNameHead,"%[1-9]",cFullJpgNameHead2);
        //cFullJpgNameHead2 = strrchr(cFullJpgNameHead,'/');
        sscanf(cFullJpgNameHead,"%[^1-9]",cFullJpgNameHead1);

        pcFullJpgNameHead2=strrchr(cFullJpgNameHead,'/');
        pcFullJpgNameHead2++;
        memcpy(cFullJpgNameHead2,pcFullJpgNameHead2,strlen(pcFullJpgNameHead2));
        sprintf(cFullJpgNameHead3,"K%s",cFullJpgNameHead2);

        memset(cFullJpgNameHead,0,sizeof(cFullJpgNameHead));
        sprintf(cFullJpgNameHead,"%s%s\n",cFullJpgNameHead1,cFullJpgNameHead3);

        //iForcedRecordTotalSec = time((time_t*)NULL) - iStartForcedRecordSec;
        iForcedRecordTotalSec = time((time_t*)NULL) - iStartForcedRecordSec - 4;

        //获取图像名 停止强制录制
        //sprintf(ucForceRecordFullName,"%s_%d.jpg",cFullJpgNameHead,iForceRecordTime);        
        sprintf(ucForceRecordFullName,"%s_%d.jpg",cFullJpgNameHead,iForcedRecordTotalSec);        
        printf("ucForceRecordFullName:%s\n",ucForceRecordFullName);
        g_ucStopForcedRecordFlag = 0x01;
        uninit_time();
       }else{
         //signal(SIGVTALRM, ForcedRecordSigSubroutine);
         signal(SIGALRM, ForcedRecordSigSubroutine);//SIGALRM
       }
    break;   
   }
   return;
}


/*
   开启定时器 定时1s
   若没有收到清除标志则停止录制
*/
int StartForcedRecord()
{
    struct itimerval value, ovalue;    

    iStartForcedRecordSec = time((time_t*)NULL);
    
    g_ucStartForcedRecordFlag = 0;
    iForceRecordTime = 1; 
    ucForceRecordTimeOutFlag = 0x00;

    //signal(SIGVTALRM, ForcedRecordSigSubroutine);//SIGALRM
    signal(SIGALRM, ForcedRecordSigSubroutine);//SIGALRM
    value.it_value.tv_sec = 1;
    value.it_value.tv_usec = 0;
    value.it_interval.tv_sec = 1;
    value.it_interval.tv_usec = 0;
    //setitimer(ITIMER_VIRTUAL, &value, &ovalue);
    setitimer(ITIMER_REAL, &value, &ovalue);

    cdr_capture_jpg(0);
  
    return 0;
}



int cdr_lt8900_cb_pro(int iKeyValue)
{
    int iTempValue = 0;
    printf("iKeyValue:%02x\n",iKeyValue);

    iTempValue = read_data_gpio_value();

    switch(iKeyValue)
    {
		case SINGLE_PRESS:
		if(iTempValue == 0x01)		//来电状态
		{
			cdr_bt_cmd(CDRBT_CMD_PHONE);//挂断/接听电话(同一个命令)			
		}else{          
			cdr_uart2_printf("$TEST,2.4G,TRIGGER\r\n");
			cdr_capture_jpg(0);  
			cdr_AssociatedVideo(g_cdr_systemconfig.photoWithVideo,CDR_CUTMP4_EX_TIME);
		}
     break;
     case LONG_PRESS:
        if(iTempValue == 0x01)
        {
          cdr_bt_cmd(CDRBT_CMD_REJECT);
        }else{

		printf("LONG_PRESS \n");
		//强制录视频 
		cdr_AssociatedVideo(1,CDR_CUTMP4_EX_TIME);
		cdr_play_audio(CDR_AUDIO_DIDI,0);
          //StartForcedRecord();         
          //printf("StartForcedRecord...\n");
        }
        break;
     case DOUBLE_PRESS:
        if(iTempValue == 0x01)
        {
          cdr_bt_cmd(CDRBT_CMD_CUT);//长按1S语音切换到手机，再长按切换到蓝牙
        }else{          
          cdr_bt_cmd(CDRBT_CMD_PHONE); //空闲状态单击，激活手机语音助手（需手机支持）
        }

        break;
     case HOLD_KEY:  
		printf("HOLD_KEY \n");
		cdr_AssociatedVideo(1,CDR_CUTMP4_EX_TIME);		
		cdr_play_audio(CDR_AUDIO_DIDI,0);
        break;
     default:
        break;     
    }
    return 0;
}

static unsigned char g_ucBmaAssoVideFlag = 0;

unsigned char GetBmaAssoVideFlagValue()
{
    return g_ucBmaAssoVideFlag;
}

unsigned char SetBmaAssoVideFlagValue(unsigned char ucValue)
{
    g_ucBmaAssoVideFlag = ucValue;
    
    return 0;
}

int cdr_bma250_cb_pro(unsigned int uiRegValue)
{  
    cdr_capture_jpg(1); 
    cdr_AssociatedVideo(2,CDR_CUTMP4_EX_TIME);
    SetBmaAssoVideFlagValue(0x01);
	cdr_uart2_printf("$TEST,GSENSOR,TRIGGER\r\n");
	return 0;
}

//retur 1 pown on
int cdr_get_powerflag(void)
{
	return g_cdr_device_app_stop_flag;
}

int cdr_system_wait(void)
{
	while(cdr_get_powerflag())		
	{
		sleep(1);
	}
	return 0;
}

int cdr_stop_check_cb_pro(unsigned int uiRegValue)
{
    if(uiRegValue != 0x00) 
    {
      g_cdr_device_app_stop_flag = CDR_POWER_ON;
      return -1;
    }   	
	printf("******************************************\r\n");
	printf("***********CDR USB POWER OFF**************\r\n");
	printf("******************************************\r\n");
	//Tell test.
	//cdr_system("echo -e \"10.CDR USB POWER OFF \r\n \" >> /dev/ttyAMA2");
	        
    g_cdr_device_app_stop_flag = CDR_POWER_OFF;   //tell app
	
    return 0;
}

int cdr_key_check_cb_pro(unsigned int uiRegValue)
{
    
    if(uiRegValue != 0x00)   return -1;   
	cdr_play_audio(CDR_AUDIO_RESETSYSTEM,0);
    printf("key press down.\n");
    cdr_system_reset();
    
    return 0;
}

/*gps 校时*/
int GpsCalibTime(GPS_INFO *GPS)
{
    unsigned char ucTimeBuff[15] = {0};
    if(ucCalibTimeFlag == 0x00)
    {
       cdr_play_audio(CDR_AUDIO_SETTIMEOK,0);
       ucCalibTimeFlag = 0x01;
       sprintf(ucTimeBuff,"%ld%02d%02d%02d%02d%02d",
        GPS->D.year,GPS->D.month,GPS->D.day,GPS->D.hour,GPS->D.minute,GPS->D.second);
       printf("ucTimeBuff:%s\n",ucTimeBuff);           
       update_system_time(ucTimeBuff,strlen(ucTimeBuff));
    }

    return 0;
}
/*
* analyze gps data get the use informnation
*/
int cdr_uart_cb_pro(char *pRevcBuff)
{
    int res = 0;
    static unsigned char ucDstBag[200] = {0};
       
    res = uart_get_dst_bag(stUartQueue.pucBuf,ucDstBag);
    if(res == 0)
    {
        gps_rmc_parse(ucDstBag,&g_GpsInfo);
        show_gps(&g_GpsInfo);
        
        GpsCalibTime(&g_GpsInfo);        
        
        memcpy(g_ucGpsDataBuff,ucDstBag,strlen(ucDstBag)+1);
        cdr_add_gp(ucDstBag);
        memset(ucDstBag,0,sizeof(ucDstBag));
        res = 0;        
    }else{
        res = -1;
    }

	//For factory test.
	if(pRevcBuff && strstr(pRevcBuff,"$TEST,START"))
	{
		cdr_uart2_printf("$TEST,START,OK\r\n");
	}
	
    return res;
}

int cdr_device_init(void)
{
    unsigned short usDeviceID = 0x9ea3;       

	cdr_uart_init();
    cdr_uart_setevent_callbak(cdr_uart_cb_pro);	

	cdr_start_factest();

	cdr_bt_init();
	
    cdr_lt8900_init(usDeviceID);
    cdr_lt8900_setevent_callbak(cdr_lt8900_cb_pro);
    
    cdr_stop_check_init();
    cdr_stop_check_setevent_callbak(cdr_stop_check_cb_pro);

	cdr_bma250_init(0x01);//设置加速度量程16g
    cdr_bma250_setevent_callbak(cdr_bma250_cb_pro);
    cdr_bma250_mode_ctrl(g_cdr_systemconfig.accelerationSensorSensitivity); 


    cdr_led_init();
    //cdr_led_contr(LED_RED,LED_STATUS_DFLICK);
    //cdr_led_contr(LED_BLUE,LED_STATUS_DFLICK);
	cdr_uart2_printf("$TEST,LED,OPEN\r\n");	

    cdr_key_check_init();
    cdr_key_check_setevent_callbak(cdr_key_check_cb_pro);

    cdr_qn8027_config();
    cdr_set_qn8027_freq(g_cdr_systemconfig.fmFrequency);//100*100 xM*100  10000 此值 应从文件中读取要修改
	cdr_uart2_printf("$TEST,FM,OPEN\r\n");
		
    cdr_bt_get_status();

	if(g_sdIsOnline == 1)
		cdr_uart2_printf("$TEST,SD,OK\r\n");

	//cdr_system("/home/goahead/goahead -v --home /home/goahead/ /mnt/mmc &");
}


int cdr_device_deinit(void)
{
	cdr_play_audio(CDR_AUDIO_USBOUT,0);
	cdr_add_log(eSTOP_CDR, NULL,NULL);
	cdr_uart_deinit();
    cdr_add_end_gp_record();
	cdr_system("ifconfig wlan0 down");
	cdr_bt_deinit();
	cdr_lt8900_close();
	cdr_rec_realease();
	cdr_mp4dirlist_free();
	sleep(4);          	//Just sleep 4 seconds.  wait for save to sd.
	cdr_unload_sd();
	printf("[%s %d]\r\n",__FUNCTION__,__LINE__);
       
}

//param: reg GPIO_DIR value.
//nIndex:0-7 GPIOx_0 - GPIOx_7
//mode:0 input 1 output.
int cdr_set_gpio_mode(unsigned int reg,int nIndex,int mode)
{
	//只能设定GPIO_DIR值.
	if(reg & 0x0400 != 0x0400)
		return -1;

	unsigned int nValue;
	if(HI_SUCCESS == HI_MPI_SYS_GetReg(reg,&nValue))
	{
		if(mode == 1)
		{
			nValue |= (0x01<<nIndex);
		}
		else
		{
			nValue &= ~(0x01<< nIndex);
		}
		HI_MPI_SYS_SetReg(reg,nValue);
	}
	return -1;	
}

//设置一个标志位.生产测试软件可用.
int cdr_start_factest(void)
{
	char ch[100];
	char mac[14];
	char chTM[20];
	

	struct tm tm_now;	
	snprintf(ch,sizeof(ch),"%s %s",__DATE__,__TIME__);
	strptime(ch, "%b %d %Y %H:%M:%S", &tm_now); 
	strftime(chTM, sizeof(chTM),"%Y%m%d%H%M%S", &tm_now);
	//strftime(ch, sizeof(ch),"%Y%m%d%H%M%S", &tm_now);
	memset(ch,0x00,sizeof(ch));
	cdr_get_host_mac(mac);
	snprintf(ch,sizeof(ch),"$TEST,DEVICE,%s,%s,%s,%s\r\n",g_cdr_systemconfig.sCdrNetCfg.apSsid,mac,CDR_FW_VERSION,chTM);
	cdr_uart2_printf(ch);
	return 0;
}

