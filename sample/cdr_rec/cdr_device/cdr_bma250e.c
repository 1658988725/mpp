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


#include <errno.h>
#include <pthread.h>
#include <signal.h>  
#include <math.h>

#include "cdr_bma250e.h"
#include "hiGpioReg.h"
#include "cdr_config.h"

#include "cdr_mpp.h"

#define BMA_DEVICE_NAME "/dev/spidev1.1"

#define GPIO0_2PIN_HIGHT  0x04

typedef struct {
    float usAcc_x;
    float usAcc_y;
    float usAcc_z;
}sBma250Value;

const unsigned char ucSensModeArr[][2] =
{
    //mode  ,sensitivity
    {BMA250_MODE_CLOSE, 0x00},
    {BMA250_MODE_LOW,   0x30},//0x10 or 0x08
    {BMA250_MODE_MIDDLE,0x04},    
    {BMA250_MODE_HIGHT, 0x02},
};


typedef unsigned char u8;
typedef unsigned short u16;

static unsigned char read_bma250e(unsigned char addr);
static void write_bma250e(unsigned char addr, unsigned char byte);

static int cdr_bma250_handle_thread(void);

static unsigned char mode = SPI_MODE_3;
static unsigned char bits = 8;
static uint32_t speed = 3 * 1000 * 1000;


static cdr_bma250_callback g_bma250_fun_callbak;
sBma250Value g_sBma250ValueMumber;


#define ACC_HIGHT   12.0
#define ACC_MIDDLE  15.0
#define ACC_LOW     19.0
#define ACC_DEFAULT 15.0


//static int g_AccValue = ACC_MIDDLE;
//static float g_AccValue = ACC_MIDDLE;
static volatile float g_AccValue = ACC_MIDDLE;
static unsigned char ucRolloverFlag = 0;
static unsigned char ucRepeatFlag = 0;
static float fAverageX = 0.0,fAverageY = 0.0,fAverageZ = 0.0;
static float volatile fAverAcc2 = 0.0;

static float fAcc_x[10] = {0.0},fAcc_y[10] = {0.0},fAcc_z[10] = {0.0};



static unsigned short spi_write_register(unsigned int addr, unsigned char data)
{
	int fd = -1;
	unsigned short ret;
	unsigned int value;
	struct spi_ioc_transfer mesg[1];
	unsigned char tx_buf[4];
	unsigned char rx_buf[4];
    
	fd = open(BMA_DEVICE_NAME, 0);
	if (fd < 0) return -1;
    
	memset(tx_buf, 0, sizeof(tx_buf));
	memset(rx_buf, 0, sizeof(rx_buf));

	tx_buf[0] = addr;
	tx_buf[1] = data;
	
	memset(mesg, 0, sizeof mesg);
	mesg[0].tx_buf = (__u32)tx_buf;
	mesg[0].rx_buf = (__u32)rx_buf;
	mesg[0].len = 2;
	mesg[0].speed_hz = speed;
	mesg[0].bits_per_word = 8;
	mesg[0].cs_change = 1;

	value = mode ;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &value);
	if (ret < 0) 
	{
		close(fd);
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), mesg);
	if (ret != mesg[0].len) 
	{
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static unsigned char spi_read_register(unsigned int addr)
{
	int fd = -1;
	unsigned char  ret = 0;
	unsigned int value;
	struct spi_ioc_transfer mesg[1];
	unsigned char tx_buf[8];
	unsigned char rx_buf[8];
    
	fd = open(BMA_DEVICE_NAME, 0);
	if (fd < 0)	return -1;
    
	memset(tx_buf, 0xFF, sizeof(tx_buf));
	memset(rx_buf, 0, sizeof(rx_buf));	
	tx_buf[0] = addr | 0x80;
	memset(mesg, 0, sizeof mesg);
	mesg[0].tx_buf = (__u32)tx_buf;
	mesg[0].rx_buf = (__u32)rx_buf;
	mesg[0].len = 2;
	mesg[0].speed_hz = speed;
	mesg[0].bits_per_word = 8;
	mesg[0].cs_change = 1;
	mesg[0].delay_usecs = 1;

	value = mode ;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &value);
	if (ret < 0) 
	{
		printf("[%s,%d]\r\n",__FUNCTION__,__LINE__);
		close(fd);
		return -1;
	}
    //printf("[%s,%d]",__FUNCTION__,__LINE__); 
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), mesg);
	if (ret != mesg[0].len) 
	{
		close(fd);
		return -1;
	}
	close(fd);
    
	ret = rx_buf[1];
	
	//printf("%s:tx_buf[0]:%02x,tx_buf[1]:%02x,tx_buf[2]:%02x\r\n",__FUNCTION__,tx_buf[0],tx_buf[1],tx_buf[2]);
	//printf("%s:rx_buf[0]:%02x,rx_buf[1]:%02x,rx_buf[2]:%02x\r\n",__FUNCTION__,rx_buf[0],rx_buf[1],rx_buf[2]);
	return ret;
}

static u8 read_bma250e(u8 addr)
{
	return spi_read_register(addr);
}

static void write_bma250e(u8 addr, u8 byte)
{	
	spi_write_register(addr,byte);		
}


static void HiSpi1HardWareCfg(void)
{
    //复用SPI1的io口.
    cdr_system("himm 0x200f0050  0x01");
    cdr_system("himm 0x200f0054  0x01");
    cdr_system("himm 0x200f0058  0x01");
    cdr_system("himm 0x200f005C  0x01");//spi1_0 cs
    cdr_system("himm 0x200f00F0  0x02");//spi1_1 cs

    cdr_system("himm 0x200f0078  0x00"); //复用   g0_2 为gpio
    //cdr_system("himm 0x20140400  0x02"); 
    cdr_set_gpio_mode(0x20140400,2,0);//模式，g0_2为输入

    #if 0
    //7_4 -->GPIO 7_5 input
    cdr_system("himm 0x200f00f4  0x01");//gpio
    cdr_system("himm 0x201B0400  0x00");//input
    #endif
    usleep(10000);
}

static char softreset_bma250e(void)
{
	write_bma250e(0x14,0xB6);	
	usleep(10000);  //wait for chip finishing reset 	
	return 1;
}

static u8 write_bma250e_with_check(u8 addr, u8 byte, u8 ck_mask, u8 ret) 
{
#if 1
	u8 tick = 0;	
  	do{
        write_bma250e(addr,byte);
        usleep(10);
        if(tick++ > 3)
        {
         printf("addr:%02x,byte:%02x ERROR\r\n",addr,byte);
         return 0;
        }
	}while( (read_bma250e(addr) & ck_mask) != ret);
	//printf("addr:%02x,byte:%02x OK\r\n",addr,byte);	
#else
	write_bma250e(addr,byte);
	usleep(10000);
#endif
	return 1;
}

//进入低功耗模式1
static void Bma250eEnterLPM1(void)
{
  	write_bma250e_with_check(0x11, 0x5A, 0xff, 0x5A);  
	write_bma250e_with_check(0x12, 0x20, 0xff, 0x20);
}


//进入suspend
static void Bma250eEnterSuspend(void)
{
  	write_bma250e_with_check(0x11, 0x4A, 0xff, 0x4A);  
	//write_bma250e_with_check(0x12, 0x20, 0xff, 0x20);
}

//#define RANGE_TYPE  8 //0x02


/*
2g : 0x03 2 9.8
4g : 0x05 4 9.8
8g : 0x08 8 9.8
16g:0x0c 16 9.8
*/
#define RANGE_VALUE 0x0c 
#define RANGE_TYPE  16 
#define RESOLUTION  9.8
//eMode 此参数仅可关bma250
static void cfg_bma250e(eBMA250DeviceMode eMode)
{
  	u8 tick = 0;
    unsigned char cSensitivity = 0x03;

    cSensitivity = ucSensModeArr[eMode][1];
    
	if(cSensitivity == 0x00)
    {
        Bma250eEnterSuspend();
        return;
    }
	while(1){
	    if (tick++ > 3)  	break;
		
	  	softreset_bma250e(); //soft-reset chip
				
		//设置量程，±2g , sensitivity：3.91mg/LSB 最小精度
		//if(!write_bma250e_with_check(0x0F, 0x03, 0x0F, 0x03))  continue;
         if(!write_bma250e_with_check(0x0F, RANGE_VALUE, 0x0F, RANGE_VALUE))  continue;

         #if 0        
         //BMA250E_USE_HIGH_G_INT
		write_bma250e(0x17, 0x07);
		write_bma250e(0x19, 0x02);
         #endif

		//设置滤波带宽，31.25Hz
	  	if(!write_bma250e_with_check(0x10, 0x0C, 0x1F, 0x0C)) continue;
		
		// enable any-motion interrupt
	  	if(!write_bma250e_with_check(0x16, 0x07, 0xFF, 0x07))  continue;
		
		// enable slow-motion interrupt
	  	//if(!write_bma250e_with_check(0x18, 0x07, 0x0F, 0x07))
		 // 	continue;
		
		// 把震动中断映射到INT1引脚
	  	if(!write_bma250e_with_check(0x19, 0x04, 0xFF, 0x04)) continue;
		
		// 把slow-motion中断映射到INT2引脚
	  	//if(!write_bma250e_with_check(0x1B, 0x08, 0xFF, 0x08)) continue;
         if(!write_bma250e_with_check(0x1B, 0x02, 0xFF, 0x02)) continue;
		
	  	// 设置INT1、INT2中断输出类型为：推挽输出，在没有中断时，保持低电平，产生中断时，输出高电平脉冲。
		if(!write_bma250e_with_check(0x20, 0x05, 0x0f, 0x05))  continue;
		
		// latch interrupt pulse  for 1s
		if(!write_bma250e_with_check(0x21, 0x83, 0x0f, 0x03)) continue;
		
		//设置震动采集点数，与传感器的采集频率相关
		if(!write_bma250e_with_check(0x27, 0x03, 0x03, 0x03)) continue;
		
		  //设置震动中断的阀值, 3.91mg x 2 = 7.82mg
		//if(!write_bma250e_with_check(0x28, 0x02, 0xFF, 0x02))//cSensitivity
		if(!write_bma250e_with_check(0x28, cSensitivity, 0xFF, cSensitivity))//cSensitivity
		  	continue;		

		//Bma250eEnterLPM1();
		break;
        
	}	
}

static void init_bma250e(unsigned char eMode)
{       	
	cfg_bma250e(eMode);
}



static void ReadBma250Data(sBma250Value* sBma250eValue)
{
    unsigned char dataout[6]={0};
    unsigned char ucRangeType = RANGE_TYPE;

    dataout[0] =  read_bma250e(2);
    dataout[1] =  read_bma250e(3);
    dataout[2] =  read_bma250e(4);
    dataout[3] =  read_bma250e(5);
    dataout[4] =  read_bma250e(6);
    dataout[5] =  read_bma250e(7);
    
    sBma250eValue->usAcc_x = ((((unsigned char)dataout[0]) >> 6) | ((unsigned short)(dataout[1]) << 2) );
    if(sBma250eValue->usAcc_x > 0x1ff)//511
    {
        sBma250eValue->usAcc_x = -(0x3ff-sBma250eValue->usAcc_x);//1023
    }
    
    sBma250eValue->usAcc_x = (sBma250eValue->usAcc_x*RESOLUTION)/(0x200/ucRangeType); //当量程为±2g时，转换为g/s的加速度换算公式
    
    sBma250eValue->usAcc_y = ((((unsigned char)dataout[2]) >> 6) | ((unsigned short)(dataout[3]) << 2) );
    if(sBma250eValue->usAcc_y > 0x1ff)
    {
        sBma250eValue->usAcc_y = -(0x3ff-sBma250eValue->usAcc_y);
    }
    sBma250eValue->usAcc_y = (sBma250eValue->usAcc_y*RESOLUTION)/(0x200/ucRangeType); //当量程为±2g时，转换为g/s的加速度换算公式    sBma250eValue->usAcc_z = ( (((unsigned char)dataout[4]) >> 6) | ((unsigned short)(dataout[5]) << 2) );
    //1000 mg  9.8m/s              256

    sBma250eValue->usAcc_z = ((((unsigned char)dataout[4]) >> 6) | ((unsigned short)(dataout[5]) << 2) );
    if(sBma250eValue->usAcc_z > 0x1ff)
    {
        sBma250eValue->usAcc_z = -(0x3ff-sBma250eValue->usAcc_z);
    }
    sBma250eValue->usAcc_z = (sBma250eValue->usAcc_z*RESOLUTION)/(0x200/ucRangeType); //当量程为±2g时，转换为g/s的加速度换算公式
    //2的10次方，1024  正负/2 = 512   除量程，最小刻度   512/2
   
}

//中值滤波
float GetMedianNum(float * bArray, int iFilterLen)  
{  
    int i,j;// 循环变量  
    float bTemp;  
      
    // 用冒泡法对数组进行排序  
    for (j = 0; j < iFilterLen - 1; j ++)  
    {  
        for (i = 0; i < iFilterLen - j - 1; i ++)  
        {  
            if (bArray[i] > bArray[i + 1])  
            {  
                bTemp = bArray[i];  
                bArray[i] = bArray[i + 1];  
                bArray[i + 1] = bTemp;  
            }  
        }  
    }        
    // 计算中值  
    if ((iFilterLen & 1) > 0)  
    {  
        bTemp = bArray[(iFilterLen + 1) / 2];  // 数组有奇数个元素，返回中间一个元素  
    }else{         
        bTemp = (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 + 1]) / 2.0;   // 数组有偶数个元素，返回中间两个元素平均值  
    }      
    return bTemp;  
}  



static void ReadBma250DataAverage()
{
    int i;
    sBma250Value sBma250ValueMumber;  
    //读10 次，中间要有时间间隔
    for(i=0;i<10;i++)
    {
        ReadBma250Data(&sBma250ValueMumber);//1spi read 可以优化为一次读多个byte
        fAcc_x[i] = sBma250ValueMumber.usAcc_x;
        fAcc_y[i] = sBma250ValueMumber.usAcc_y;
        fAcc_z[i] = sBma250ValueMumber.usAcc_z;
        usleep(1000);
    }
     fAverageX = GetMedianNum(fAcc_x,10);
     fAverageY = GetMedianNum(fAcc_y,10);
     fAverageZ = GetMedianNum(fAcc_z,10);     
     fAverAcc2 = sqrt(fAverageY*fAverageY + fAverageX*fAverageX + fAverageZ*fAverageZ);
     //printf("fAverAcc2:%f\n",fAverAcc2);
}


//Register callback function.
void cdr_bma250_setevent_callbak(cdr_bma250_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n",__FUNCTION__,__LINE__);
		return;
	}    
	g_bma250_fun_callbak = callback;

    if(g_bma250_fun_callbak  != NULL)	
    {
		printf("%s register g_bma250_fun_callbak . \r\n",__FUNCTION__);
    }

}

//bma250 mode control
void  cdr_bma250_mode_ctrl(eBMA250DeviceMode eMode)
{
	if(eMode == 0x00)
	{
		//Bma250eEnterSuspend();  
         g_AccValue = ACC_LOW*1000;//设置成close无效
	}
	else
	{
        switch(eMode)
        {
         case BMA250_MODE_HIGHT:g_AccValue = ACC_HIGHT;break;
         case BMA250_MODE_MIDDLE:g_AccValue = ACC_MIDDLE;break;
         case BMA250_MODE_LOW:g_AccValue = ACC_LOW;break;
         default:g_AccValue = ACC_DEFAULT;break;
        }
	}
}

static void start_bma250_handle_thread(void) 
{ 	
	pthread_t tfid;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, (void *)cdr_bma250_handle_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }
    pthread_detach(tfid);
}



int cdr_bma250_init(unsigned char eMode)
{
    HiSpi1HardWareCfg();

	init_bma250e(eMode);
    
	start_bma250_handle_thread();

	return 1;
}


static int cdr_bma250_callback_imp(unsigned int uiRegValue)
{
    if(uiRegValue == GPIO0_2PIN_HIGHT)
    {
       printf("[%s,%d] pin0_2 value %02x \n",__FUNCTION__,__LINE__,uiRegValue);
    }
    return 1;
}

static unsigned int ReadGpioValue()
{
    unsigned int uiRegValue;

    //reg_read(0x20140010, &uiRegValue);//gpio 0_2    
    HI_MPI_SYS_GetReg(0x20140010, &uiRegValue);
    //reg_read(0x201B0080, &uiRegValue);//gpio 7_5

    //printf("uiRegValue:%d\n",uiRegValue);

    return uiRegValue;
}



int AutoFlipProcess()
{   
   int i = 0;
   //char  pRotate[5] = {0};
  
   if(fAverageX < -8.0){

     for(i=0;i<4;i++){
       SetVodieFlip(1,1,i,i);     
     }
   }
   if(fAverageX > 8.0){

     for(i=0;i<4;i++){
       SetVodieFlip(0,0,i,i);     
     }
   }

   return 0;   
}

#define BMATEST 0

#if(1)

static int cdr_bma250_handle_thread()
{
    //sBma250Value sBma250ValueMumber1;

	unsigned int uiRegValue = 0;
	unsigned char ucBuff[500] = {0};
	time_t tt;
	char tmpbuf[80];
    cdr_bma250_callback bmafcallbak = g_bma250_fun_callbak;

    int iCount = 0;
    
	while(1)
	{           
		bmafcallbak = g_bma250_fun_callbak;        
		usleep(10000);
		ReadBma250DataAverage();
       
		if(bmafcallbak && (fAverAcc2 > g_AccValue))
		{
			bmafcallbak(uiRegValue);	

#ifdef BMATEST  
			tt=time(NULL);
			strftime(tmpbuf,80,"%Y-%m-%d %H:%M:%S",localtime(&tt));
			sprintf(ucBuff,"echo \"%s set:%f,Actual acceleration: %f\n\" >> /mnt/mmc/tmp/bma.txt",tmpbuf,g_AccValue,fAverAcc2);             
			cdr_system(ucBuff);
#endif             
              sleep(1);
		}                        
        
         iCount++;  
         //printf("iCount:%d\n",iCount);
         if(iCount >= 30){
            iCount = 0;
            if(g_cdr_systemconfig.sCdrVideoRecord.rotate == 0x03)//auto mode
            {
              AutoFlipProcess();
            }
         }
	}
}

#else

static int cdr_bma250_handle_thread()
{
    sBma250Value sBma250ValueMumber1;

	unsigned int uiRegValue = 0;
	unsigned char ucBuff[500] = {0};
	time_t tt;
	char tmpbuf[80];
    cdr_bma250_callback bmafcallbak = g_bma250_fun_callbak;

	while(1)
	{           
		bmafcallbak = g_bma250_fun_callbak;        
		usleep(10000);
		ReadBma250DataAverage();

		if(bmafcallbak && (fAverAcc2 > g_AccValue))
		{
			bmafcallbak(uiRegValue);	

#ifdef BMATEST  
			tt=time(NULL);
			strftime(tmpbuf,80,"%Y-%m-%d %H:%M:%S",localtime(&tt));
			sprintf(ucBuff,"echo \"%s set:%f,Actual acceleration: %f\n\" >> /mnt/mmc/tmp/bma.txt",tmpbuf,g_AccValue,fAverAcc2);             
			cdr_system(ucBuff);
#endif             
              sleep(1);
		}                        
	}
}
#endif


