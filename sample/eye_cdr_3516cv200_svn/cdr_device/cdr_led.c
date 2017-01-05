/************************************************************************	
** Filename: 	cdr_led.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/wait.h>
#include <memory.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>  
#include "cdr_led.h"

static int cdr_led_handle_thread();

static unsigned int g_ledStatus[LED_NUMMAX];

static void led_hw_cfg(void)
{	
    //mode is gpio 
	cdr_system("himm 0x200f00d8  0x00");//d12 led gpio6_6    
	cdr_system("himm 0x201A0400  0x40");//d12 led gpio6_6
	cdr_system("himm 0x201A0100  0x20");//d12 led gpio6_6

    cdr_system("himm 0x200f00e4  0x00");//d13 yellow  gpio7_1
	cdr_system("himm 0x201B0400  0x02");//d13 yellow  gpio7_1
    cdr_system("himm 0x201B0008  0x02");//d12 led gpio6_6
        
    usleep(10000);  
}

static void start_led_handle_thread(void) 
{ 	
	pthread_t tfid = NULL;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, (void *)cdr_led_handle_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);
}


int cdr_led_init(void)
{
	//int nRet = 0;
		
	led_hw_cfg();
        
	start_led_handle_thread();

	return 1;
}

//test
//if set mode is 0x00  then  no led bright 
int cdr_led_contr(int nLedIndex,int nMode)
{
	g_ledStatus[nLedIndex] = nMode;
	return 0;
}
static int set_led_mode(int nLedIndex,int nValue)
{
	unsigned long reg = 0x00;
	if(nLedIndex == LED_RED) 
	{
		HI_MPI_SYS_SetReg(0x200f00d8,0x00);//d12 led gpio6_6    
		//HI_MPI_SYS_SetReg(0x201A0400,0x40);//d12 led gpio6_6
		cdr_set_gpio_mode(0x201A0400,6,1);
		reg = 0x201A0100;
	}	
	else if(nLedIndex == LED_BLUE)
	{
		reg = 0x201B0008;
	}

	if(reg > 0)
	{
		if(nValue > 0)
		HI_MPI_SYS_SetReg(reg,0x00);			
		else
		HI_MPI_SYS_SetReg(reg,0xFF);
	}
}

static int cdr_led_handle_thread()
{
	int i = 0;
	while(1)
	{	
		for(i=0;i<LED_CYCLE_CNT;i++)
		{
			set_led_mode(LED_RED,(g_ledStatus[LED_RED]>>i) & 0x01);
			set_led_mode(LED_BLUE,(g_ledStatus[LED_BLUE]>>i) & 0x01);
			usleep(200000);
		}
	}
}
