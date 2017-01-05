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

#include "cdr_key_check.h"

#include "hiGpioReg.h"

static cdr_key_check_callback g_key_check_fun_callbak;

static void cdr_key_check_handle_thread();

//g7_0
static void key_HiHardWareCfg(void)
{
	cdr_system("himm  0x200f00e0  0x00");//set gpio mode
    cdr_system("himm  0x201B0400  0x02");//set out mode

	usleep(100);
}

void cdr_key_check_setevent_callbak(cdr_key_check_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n");
		return;
	}
    
	g_key_check_fun_callbak = callback;

    if(g_key_check_fun_callbak  != NULL)	
    {
		printf("%s register g_key_check_fun_callbak .\r\n",__FUNCTION__);
    }

}


static void init_key_check_handle_thread(void) 
{ 	
	pthread_t tfid;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, cdr_key_check_handle_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }
    pthread_detach(tfid);
}

int cdr_key_check_init()
{

    key_HiHardWareCfg();
    
	init_key_check_handle_thread();

	return 1;
}

int cdr_key_check_callback_imp(unsigned int uiRegValue)
{  
  int ret = 0;
  
  if(uiRegValue == 0x00)
  {
    printf("[%s %d] key down\n",__FUNCTION__,__LINE__);
    //process();//power down 
  }
  
  return ret;
}

static unsigned int read_key_check_gpio_value()
{
    unsigned int uiRegValue = 0;
    
    HI_MPI_SYS_GetReg(0x201b0004  , &uiRegValue);//GPIO8_1
    //printf(" regvalue %d\n",uiRegValue);
    
    return uiRegValue;
}

static void cdr_key_check_handle_thread()
{
    unsigned int uiRegValue = 0,uiRegValue1 = 0;
    
    cdr_key_check_callback keyfcallbak = g_key_check_fun_callbak;

    while(1)
    {   
        keyfcallbak = g_key_check_fun_callbak;
        usleep(300000); 		
        uiRegValue = read_key_check_gpio_value();
		
		if(uiRegValue == 0x00)
			cdr_uart2_printf("$TEST,RESETKEY,TRIGGER\r\n");
			
		sleep(3);
		uiRegValue1 = read_key_check_gpio_value();	//Ïû¶¶	

        if(keyfcallbak && uiRegValue == uiRegValue1)
        {
			keyfcallbak(uiRegValue);				
		}         
	}
}


