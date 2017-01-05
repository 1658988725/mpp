/************************************************************************	
** Filename: 	bma250e.c
** Description:  driver for acceleration sensor bma250e
** Author: 	xjl
** Create Date: 2016-6-21
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
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <string.h> 
#include <errno.h>
#include <pthread.h>
#include <signal.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>


#include "cdr_bma250e.h"
#include "cdr_factory_test.h"
#include "cdr_bt.h"
#include "cdr_gps_data_analyze.h"
#include "cdr_led.h"
#include "cdr_lt8900.h"

#include "cdr_comm.h"
int _uart2_fd = 0;
int cdr_key_cb_pro_factory(unsigned int uiRegValue)
{
    if(uiRegValue != 0x00) 
    {
      return -1;
    }   

	 cdr_uart2_printf("$TEST,RESETKEY,TRIGGER\r\n");
    //cdr_factory_out("4.KEY OK");
    return 0;
}

int cdr_bma250_cb_pro_factory(unsigned int uiRegValue)
{  
	static int flag = 0;
 	if(uiRegValue == 0x04 && flag == 0)
	{
		flag = 1;
		//cdr_factory_out("5.BMA250 OK");
	}  
	return 0;
}

int cdr_getmac(char *pName)
{ 
    struct   ifreq   ifreq; 
    int   sock; 
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
        //perror( "socket "); 
        return   2; 
    } 
    strcpy(ifreq.ifr_name,"wlan0"); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        //perror( "ioctl "); 
        return   3; 
    } 
	char strMAC[13];
	memset(strMAC,0x00,13);

	sprintf(strMAC,"%02X%02x%02X%02X%02X%02X", 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[0], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[2], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]); 
	memcpy(pName,strMAC+7,5);
    return   0; 
}

int cdr_wifi_factory(void)
{
	char chWifiName[20];
	char chMacInfo[13];
	memset(chMacInfo,0x00,sizeof(chMacInfo));
	memset(chWifiName,0x00,sizeof(chWifiName));
	if(cdr_getmac(chMacInfo) != 0)		
		return -1;
	
	sprintf(chWifiName,"KAKA_%s",chMacInfo);

	char cSedApSsidCmd[150] = {0};
	sprintf(cSedApSsidCmd,"sed -i '/EncrypType/ aiwpriv wlan0 set SSID=\"%s\"' /home/wifi_bin/init_wifi_7601.sh",chWifiName);
	cdr_system("sed -i '/SSID/d' /home/wifi_bin/init_wifi_7601.sh");
	cdr_system("sync");
	cdr_system(cSedApSsidCmd);
	cdr_system("sync");

	sprintf(cSedApSsidCmd,"8.WIFI SSID:%s\r\n",chWifiName);
	//cdr_factory_out(cSedApSsidCmd);


	//cdr_system("sh /home/wifi_bin/init_wifi_7601.sh");
	sprintf(cSedApSsidCmd,"iwpriv wlan0 set SSID=\"%s\"",chWifiName);
	cdr_system("ifconfig wlan0 down");
	cdr_system("ifconfig wlan0 192.168.100.2");
	cdr_system("iwpriv wlan0 set AuthMode=WPA2PSK");
	cdr_system("iwpriv wlan0 set EncrypType=AES");
	cdr_system(cSedApSsidCmd);
	cdr_system("iwpriv wlan0 set WPAPSK=\"12345678\"");
	cdr_system("ifconfig wlan0 192.168.100.2 up");	
	return 0;
}

static int factoryCheckSDStatus()
{
    struct stat st;
    if (0 == stat("/dev/mmcblk0", &st))
    {
        if (0 == stat("/dev/mmcblk0p1", &st))
        {
            //printf("...load TF card success...\n");
            return 1;
        }
        else
        {
            //printf("...load TF card failed...\n");
            return 2;
        }
    }
    return 0;
}


int cdr_sd_factory()
{
	int sdIsOnline = 0;
    sdIsOnline = factoryCheckSDStatus();
    if (sdIsOnline == 1) //index sd card inserted.
    {
        mkdir(SD_MOUNT_PATH, 0755);
        //system("umount /mnt/mmc/");
        cdr_system("mount -t vfat -o rw /dev/mmcblk0p1 /mnt/mmc/"); //mount SD.
        usleep(1000000);
		return 0;	
    }
	return -1;
}


void QN8027_Write_Freq(unsigned int Frequnt)	
{

	char QN8027_Send[5];
	
    QN8027_Send[0x00] = 0x00;
    QN8027_Send[0x02] = (Frequnt - 7600)/5;
    QN8027_Send[0x01] = ((Frequnt - 7600)/5 >> 8)|0x20;

	printf("QN8027_Send[0x01] %02x QN8027_Send[0x02]%02X\r\n",QN8027_Send[0x01],QN8027_Send[0x02]);
	
   // I2C_Data_Write(0x58, &QN8027_Send[0], 3);
}

int cdr_fm_factory(void)
{
	//∏¥”√I2C
	system("himm 0x200f0060 0x1");
	system("himm 0x200f0064 0x1");
	sleep(1);

	
	system("i2c_write 0x02 0x58 0x00 0x81");
	usleep(20000);
	system("i2c_write 0x02 0x58 0x03 0x20");
	system("i2c_write 0x02 0x58 0x04 0x21");
	//system("i2c_write 0x02 0x58 0x03 0x50");
	//usleep(20000);
	//system("i2c_write 0x02 0x58 0x04 0x33");
	//usleep(20000);
	system("i2c_write 0x02 0x58 0x00 0x41");
	system("i2c_write 0x02 0x58 0x00 0x01");
	usleep(20000);
	
	system("i2c_write 0x02 0x58 0x18 0xE4");
	system("i2c_write 0x02 0x58 0x1B 0xF0");

	//…Ë÷√∆µ¬ 88M 76M+382(0x17E)*0.05 = 76+19.1 = 95.1
	system("i2c_write 0x02 0x58 0x01 0x7E");
	system("i2c_write 0x02 0x58 0x02 0xB9");
	system("i2c_write 0x02 0x58 0x00 0x22");


	printf("\n\n\n\n\n");
	
	QN8027_Write_Freq(10000);
	system("i2c_write 0x02 0x58 0x01 0xE0");
	system("i2c_write 0x02 0x58 0x00 0x21");

#if 0	
	while(1)
	{
		system("i2c_read 0x02 0x58 0x07");
		sleep(2);
	}
#endif
	
	return 0;
}

int cdr_led_factory()
{
	int i = 0;
	//cdr_factory_out("3.LED TEST");
	cdr_system("himm 0x200f00E4 0x00");
	//cdr_system("himm 0x201B0400 0x02");	
	cdr_set_gpio_mode(0x201B0400,1,1);
	
	cdr_system("himm 0x200f00E4 0x00");
	cdr_set_gpio_mode(0x201A0400,6,1);

	//cdr_system("himm 0x201A0400 0x40");
	
	for(i=0;i<6;i++)
	{
		cdr_system("himm 0x201B0008 0x02");
		cdr_system("himm 0x201A0100 0x40");
		usleep(500000);
		cdr_system("himm 0x201B0008 0x00");
		cdr_system("himm 0x201A0100 0x00");
		usleep(500000);
	}

}

//factory test mode.
int cdr_factory_mode_test(void)
{
	//init uart.
	_uart2_fd = cdr_uart_init_factory();

	char ch[200];
	sprintf(ch,"Software Compiled Time: %s %s\r\n",__DATE__, __TIME__);	
	//cdr_factory_out(ch);
	

	printf("[%s] start\n",__FUNCTION__);
	//com 1
	//cdr_factory_out("1.COM OK");
	
	//sd 2
	if(0 == cdr_sd_factory())
	{
		if (0 == access("/mnt/mmc/cdr_factory_file.bin", W_OK))
		{
			//cdr_factory_out("2.SD OK");			
		}
		else
		{
			//cdr_factory_out("2.SD ERROR*************************");
			close(_uart2_fd);
			_uart2_fd = 0;
			return 0;
		}
	}
	else
	{
		//cdr_factory_out("2.SD ERROR*************************");
	}
	

	cdr_led_factory();
	
	//KEY test.	
 	cdr_key_check_init();
    cdr_key_check_setevent_callbak(cdr_key_cb_pro_factory);
	//cdr_factory_out("KEY test.");

#if 0
	//G-sensor 5 
	cdr_bma250_init(0x01);
    cdr_bma250_setevent_callbak(cdr_bma250_cb_pro_factory);
    cdr_bma250_mode_ctrl(BMA250_MODE_HIGHT); 
	cdr_factory_out("G-sensor test.");
#endif

	//lt8900 6
	if(cdr_lt8900_factory_test() == 0)
	{
		//cdr_factory_out("6.LT8900 OK");
	}
	else
	{
		//cdr_factory_out("6.LT8900 ERROR*************************");
	}

	sleep(1);
	cdr_fm_factory();
	sleep(1);
	
	//bt 7
	cdr_bt_init();
	cdr_bt_cmd(CDRBT_CMD_OPEN);
	sleep(1);
	cdr_bt_cmd(CDRBT_CMD_STOP);
	sleep(1);
	//cdr_factory_out("7.BT test.");

	//WIFI 8	
	if(0 != cdr_wifi_factory())
	{
		//cdr_factory_out("8.Wifi ERROR*************************");		
	}
		
	
	//audio out		

	//video out.	
		
	//FM
	//mic

	close(_uart2_fd);
	_uart2_fd = 0;
	//sleep(5);
	printf("[%s] end\n",__FUNCTION__);
}
