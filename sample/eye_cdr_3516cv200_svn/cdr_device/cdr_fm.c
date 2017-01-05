/************************************************************************	
** Filename: 	cdr_fm.c
** Description:  
1，设置频点
2，当ble有数据过来时，控制开fm
3，当ble无数据过来时，控制关fm
4, 当ble无数据时，开fm，用手机搜时会有噪声吗，如何关掉此声音
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

#define FM_CLOSE  0x02
#define FM_OPEN  0x01

static int g_iFrequnt = 760;

int set_fm_frequnt(int iFrequnt)
{    
   g_iFrequnt = iFrequnt;

   return 0;
}

int get_fm_frequnt()
{
  return g_iFrequnt;
}

/*
input argument sw 
0x01 :open
0x02 :close 
*/
int cdr_qn8027_contrl(char sw)
{
  int iFrequnt = get_fm_frequnt();
  
  if(sw == FM_OPEN)//open
  {
    cdr_qn8027_config();
    cdr_set_qn8027_freq(iFrequnt);
  }
  if(sw == FM_CLOSE)//close
  {
	cdr_system("i2c_write 0x02 0x58 0x00 0x81");
  }
}

int cdr_qn8027_config(void)
{
	//复用I2C
	cdr_system("himm 0x200f0060 0x1");
	cdr_system("himm 0x200f0064 0x1");
	sleep(1);    
    //config ic regsiter
	cdr_system("i2c_write 0x02 0x58 0x00 0x81");//将QN8027所有寄存器复位到缺省值
	usleep(20000);

#if 1    
	cdr_system("i2c_write 0x02 0x58 0x03 0x10");//此值需根据硬件配置，设置QN8027为外部正旋波时钟输入( 与硬件设计相关)
	cdr_system("i2c_write 0x02 0x58 0x04 0x33");//设置12MHz时钟频率( 与硬件设计相关)
#else
	cdr_system("i2c_write 0x02 0x58 0x03 0x20");//此值需根据硬件配置，设置QN8027为外部正旋波时钟输入( 与硬件设计相关)
	cdr_system("i2c_write 0x02 0x58 0x04 0x21");//设置12MHz时钟频率( 与硬件设计相关)
#endif

	cdr_system("i2c_write 0x02 0x58 0x00 0x41");//QN8027有限状态机校验
	cdr_system("i2c_write 0x02 0x58 0x00 0x01");// QN8027有限状态机校验
	usleep(20000);	
	cdr_system("i2c_write 0x02 0x58 0x18 0xE4");//改善信噪比SNR
	cdr_system("i2c_write 0x02 0x58 0x1B 0xF0");//使QN8027发射功率最大Increase RF power output

	return 0;
}

/*
 freq:  76M-108M
 match: 760-1080
*/
int cdr_set_qn8027_freq(int iFreq)
{
    unsigned char QN8027_Send[3] = {0};
    unsigned char ucCmd1[100] = {0};
    unsigned char ucCmd2[100] = {0};

    int iFrequnt = iFreq*10;
    if(iFrequnt == 0)
    {
       //r_qn8027_contrl(FM_CLOSE);
       //cdr_set_qn8027_freq(900);
       cdr_system("i2c_write 0x02 0x58 0x00 0x12");//不发射
       return 0;
    }
    if((iFreq < 760) || (iFreq > 1080))
    {
        printf("[%s %d] invalid argument!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    
	cdr_system("i2c_write 0x02 0x58 0x01 0x7E");//set channel
	cdr_system("i2c_write 0x02 0x58 0x02 0xB9");//放弃QN8027 PA关闭功能当没有音频信号输入时
	cdr_system("i2c_write 0x02 0x58 0x00 0x22");//发射

    QN8027_Send[2] = (iFrequnt - 7600)/5;
    QN8027_Send[1] = ((iFrequnt - 7600)/5 >> 8)|0x20;

    //set fre 100M 设置发射频点：通过寄存器 Reg00h[1:0] 和 Reg01h[7:0]设置发射频率
    sprintf(ucCmd1,"i2c_write 0x02 0x58 0x01 0x%02X",QN8027_Send[0x02]);
    sprintf(ucCmd2,"i2c_write 0x02 0x58 0x00 0x%02X",QN8027_Send[0x01]);//设置为发射模式，寄存器 00H 的 bit5 设置为 1。

    cdr_system(ucCmd1);
    cdr_system(ucCmd2);

    set_fm_frequnt(iFrequnt);

    return 0;
}



