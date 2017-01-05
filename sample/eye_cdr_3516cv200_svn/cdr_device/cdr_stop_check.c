/************************************************************************	
** Filename: 	cdr_stop_process.c
** Description:
** Author: 	xjl
** Create Date: 2016-8-23
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

#include "cdr_stop_check.h"

#include "hiGpioReg.h"

static cdr_stop_check_callback g_stop_check_fun_callbak;

static void cdr_stop_check_handle_thread();

static void HiHardWareCfg(void)
{
	cdr_system("himm 0x200f0104  0x01");//gpio 8_1 复用为普通io 8_1

    //cdr_system("himm 0x201C0400  0x00"); //模式，g8_1为输入
    
	cdr_set_gpio_mode(0x201C0400,1,0);

	usleep(100);
}

void cdr_stop_check_setevent_callbak(cdr_stop_check_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n");
		return;
	}
    
	g_stop_check_fun_callbak = callback;

    if(g_stop_check_fun_callbak  != NULL)	
    {
		printf("%s register g_stop_check_fun_callbak. \r\n",__FUNCTION__);
    }

}


static void init_stop_check_handle_thread(void) 
{ 	
	pthread_t tfid;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, cdr_stop_check_handle_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }
    pthread_detach(tfid);
}

int cdr_stop_check_init()
{

    HiHardWareCfg();
    
	init_stop_check_handle_thread();

	return 1;
}

int cdr_stop_check_callback_imp(unsigned int uiRegValue)
{  
  int ret = 0;
  
  if(uiRegValue == 0x00)
  {
    printf("[%s %d] power down\n",__FUNCTION__,__LINE__);
    //process();//power down 
  }
  
  return ret;
}

static unsigned int read_power_check_gpio_value()
{
    unsigned int uiRegValue = 0;
    
    HI_MPI_SYS_GetReg(0x201C0008, &uiRegValue);//GPIO8_1
    //printf("stop check regvalue %d\n",uiRegValue);
    
    return uiRegValue;
}

static void cdr_stop_check_handle_thread()
{
    unsigned int uiRegValue = 0,uiRegValue1 = 0;
    
    cdr_stop_check_callback stopfcallbak = g_stop_check_fun_callbak;
	HiHardWareCfg();

    while(1)
    {  		
        stopfcallbak = g_stop_check_fun_callbak;

        //sleep(2); 
        usleep(300000); 
		
        uiRegValue = read_power_check_gpio_value();
		usleep(10000);
		uiRegValue1 = read_power_check_gpio_value();		

        if(stopfcallbak && uiRegValue == uiRegValue1)
        {
			stopfcallbak(uiRegValue);				
		}         
	}
}


