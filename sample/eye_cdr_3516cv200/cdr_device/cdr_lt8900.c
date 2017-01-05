/************************************************************************	
** Filename: 	cdr_lt8900.c
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

#include <net/if.h> 

#include <signal.h>  

#include "cdr_lt8900.h"

#include "cdr_comm.h"

#define LT8920_DEBUG       0
#define KEY_VALUE_SINGLE   0x01
#define KEY_VALUE_DOUBLE   0x02
#define DATA_LEN           0x08
#define ENCPRYPT_ENABLE    0x01 //0x01

//---lt8900---------------------------------------------------
typedef unsigned char u8;
typedef unsigned short u16;

#define DEVICE_NAME "/dev/spidev1.0"
static unsigned char mode = SPI_MODE_1;
static unsigned char bits = 8;
static uint32_t speed =  1000000;  

#define CHANNEL  0x28
const u8 LT_SYNCWORD[8] = {0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x78, 0x79}; 
#define PKT_FLAG (0x01<<6)
#define FIFO_FLAG (0x01<<5)
#define ACK_BUFFER_LEN 6
#define RECVDATA_LEN 9
unsigned char g_ackbuffer[ACK_BUFFER_LEN];
unsigned char g_recvBuffer[20];
unsigned char g_recvbufferlen = 0;
unsigned char g_pairflag = 0;
#define LT8920_PAIR_DATA 0xABCD
//--end of lt8900-------------------------------------------------

typedef struct
{
	unsigned char type;
	unsigned char cmd_type;
	unsigned short pair_key;    
	unsigned char key;
	unsigned char res;	
}LT8920_protocol;
LT8920_protocol g_protocoldata;

static u8 Is_LT_RecvData(void);
static unsigned char LT_Snd_Data(unsigned char *data, unsigned char len);
static unsigned char JugdeRecvbuff(void);
static void print_fifo_len(void);
static void reset_match_mode(int sig);
static int cdr_lt8900_handle_thread();

unsigned short  g_usDevceID = 0x1111;//test default
unsigned char g_cMode ;
unsigned char ucMatchStatu = 0;
static cdr_lt8900_callback g_lt8900_fun_callbak;


//SPI黍迡16bit
static unsigned short spi_write_register(unsigned int addr, unsigned char datah,unsigned char datal)
{
	int fd = -1;
	unsigned short ret;
	unsigned int value;
	struct spi_ioc_transfer mesg[1];
	unsigned char tx_buf[4] = {0};
	unsigned char rx_buf[4];
	fd = open(DEVICE_NAME, 0);
	if (fd < 0) 
	{
		return -1;
	}
	memset(tx_buf, 0, sizeof(tx_buf));
	memset(rx_buf, 0, sizeof(rx_buf));

	tx_buf[0] = addr;
	tx_buf[1] = datah;
	tx_buf[2] = datal;
	memset(mesg, 0, sizeof mesg);
	mesg[0].tx_buf = (__u32)tx_buf;
	mesg[0].rx_buf = (__u32)rx_buf;
	mesg[0].len = 3;
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

//SPI?芍D∩?角??℅??迆.
static unsigned short spi_write_register1(unsigned int addr,unsigned char *pBufer,int len)
{
	int fd = -1;
	unsigned short ret = 0;
	unsigned int value;
	struct spi_ioc_transfer mesg[1];
	unsigned char tx_buf[20];
	unsigned char rx_buf[20];
	fd = open(DEVICE_NAME, 0);
	if (fd < 0) 
	{
		return -1;
	}
	memset(tx_buf, 0x00, sizeof(tx_buf));
	memcpy(tx_buf+1,pBufer,len);
	memset(rx_buf, 0x00, sizeof(rx_buf));	
	tx_buf[0] = addr;
	memset(mesg, 0, sizeof mesg);
	mesg[0].tx_buf = (__u32)tx_buf;
	mesg[0].rx_buf = (__u32)rx_buf;
	mesg[0].len = len+1;
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
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), mesg);
	if (ret != mesg[0].len) 
	{
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static unsigned short spi_read_register(unsigned int addr)
{
	int fd = -1;
	unsigned short ret = 0;
	unsigned int value;
	struct spi_ioc_transfer mesg[1];
	unsigned char tx_buf[8];
	unsigned char rx_buf[8];
    
	fd = open(DEVICE_NAME, 0);
	if (fd < 0) 
	{
		return -1;
	}
	memset(tx_buf, 0xFF, sizeof(tx_buf));
	memset(rx_buf, 0, sizeof(rx_buf));	
    
	tx_buf[0] = addr+0x80;
	memset(mesg, 0, sizeof mesg);
	mesg[0].tx_buf = (__u32)tx_buf;
	mesg[0].rx_buf = (__u32)rx_buf;
	mesg[0].len = 3;
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
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), mesg);
	if (ret != mesg[0].len) 
	{
		close(fd);
		return -1;
	}
	close(fd);
	ret = rx_buf[1];
	ret = ret << 8;
	ret += rx_buf[2];	
	return ret;
}


static void LT_writereg(u8 reg, u8 h, u8 l)
{
	spi_write_register(reg,h,l);
}

static u16 LT_readreg(u8 reg)
{
	return spi_read_register(reg);	
}

static void LT_Delayms(unsigned long n)
{
    usleep(n*1000);
}

static void LT_TxDataEnable(void)
{
    LT_writereg(7,0x00, 0x00); //???D
    LT_writereg(52,0x80, 0x00); //????﹞⊿?赤?o∩???
}

static void LT_RxDataEnable(void)
{
  	LT_writereg(52,0x00,0x80); //???車那??o∩???
  	LT_writereg(51,0x00,0x80); //???車那??o∩???
	LT_writereg(7,0,0x80+CHANNEL); //℅?㊣??車那邦 f ?  CHANNEL
}

static unsigned char LT_Snd_Data(unsigned char *data, unsigned char len)
{
    unsigned char key;
    unsigned char val;
    //unsigned char sum1 = 0;
    unsigned short sum = 0;
    unsigned char index = 0;
	unsigned char buffer[20] = {0};

    LT_TxDataEnable();//那1?邦﹞⊿?赤

    key = (unsigned char)(rand() & 0xFF);
    sum = len+2;//℅邦3∟?豕=那y?Y3∟?豕+?邦??+角??車o赤 ?a??米?﹞?車D米??那足a㏒?車?mcu??車D1?
    //sum = len + 3;//℅邦3∟?豕=那y?Y3∟?豕 + ?邦?? + 3∟?豕 + 角??車o赤
    
    buffer[index++] = sum;	
	buffer[index++] = key;
    
	sum += key;
    while(len--)
    {
        val  = *data++;
        sum += val;//???-那y?Y??DD角??車o赤
        #if ENCPRYPT_ENABLE
        val ^= key;//???-那y?Y??DD辰足?辰    
        #endif
        buffer[index++] = val;	        
    }
	buffer[index++] = (unsigned char)sum;

#if LT8920_DEBUG
	int i = 0;
	printf("LT_Snd_Data ");
	for(i = 0 ;i < index;i++)
	{
		printf("%02x ",buffer[i]);
	}
	printf("\n");
#endif
	spi_write_register1(50,buffer,index);
	//print_fifo_len();
	
    LT_writereg(7, 0x01, CHANNEL);
    while(0 == Is_LT_RecvData()); //米豕∩y﹞⊿?赤赤那㊣?
    
    LT_RxDataEnable();
    return len;
}

static void print_fifo_len(void)
{
#if LT8920_DEBUG
	unsigned short len;
	len = LT_readreg(52);
	len >>= 8;
	len &= 0x3F;
	printf("FIFO data len:%d\r\n",len);
#endif	
}

static unsigned char LT_RecvData(void)
{
    unsigned short tmp,t48,t50;
  	unsigned char cnt,retv = 0xff;	
	//unsigned char key,val,sum;
	int index = 0;	

	tmp = LT_readreg(52);
	tmp >>= 8;
	tmp &= 0x3F;
	cnt = tmp;      //FIFO℅邦3∟?豕

	if(cnt>15||cnt<3) return retv;
	
	LT_writereg(7,0x00,0x00);	
    
	t48 = LT_readreg(48);
	if(t48 & 0x8000)return 0xFF;//printf("crc error\r\n");

	if(tmp == RECVDATA_LEN)
	{		
		memset(g_recvBuffer,0,20);
		g_recvbufferlen = 0;
        
		t50 = LT_readreg(50);
		g_recvBuffer[index++] = (t50>>8) & 0xFF;
		g_recvBuffer[index++] = t50 & 0xFF;

		t50 = LT_readreg(50);
		g_recvBuffer[index++] = (t50>>8) & 0xFF;
		g_recvBuffer[index++] = t50 & 0xFF;

		t50 = LT_readreg(50);
		g_recvBuffer[index++] = (t50>>8) & 0xFF;
		g_recvBuffer[index++] = t50 & 0xFF;
	
		t50 = LT_readreg(50);
		g_recvBuffer[index++] = (t50>>8) & 0xFF;
		g_recvBuffer[index++] = t50 & 0xFF;
        
		t50 = LT_readreg(50);
		g_recvBuffer[index++] = (t50>>8) & 0xFF;
		g_recvBuffer[index++] = t50 & 0xFF;

		g_recvbufferlen = index;

#if 0
		int i=0;
        printf("[%s,%d]",__FUNCTION__,__LINE__); 
        printf("g_recvBuffer:");
		for(i=0;i<index-1;i++)
		{
			printf("%02x ",g_recvBuffer[i]);
		}
        printf("\n");             
#endif		

		return JugdeRecvbuff();		
	}
	return retv;
}

static unsigned char ucMatchFlag = 0;

static unsigned short g_usDeviceIDTempRev = 0 ;
/*
* CRC Buffer. 那y?Y3∟?豕 + key(?邦??) + 6℅??迆那y?Y + sum
  6℅??迆: 赤﹞  ??芍?  米??﹞1 米??﹞2  ～∩???米  ?芍那???
       0xa5   0x81                       0xae
*/
static unsigned char JugdeRecvbuff(void)
{
	int i = 0;
	unsigned short sum;
    unsigned char key;
    unsigned char val;
	unsigned char ucSum;
    unsigned short usDeviceIDTemp = 0 ;

    sum = g_recvBuffer[0];
	sum += g_recvBuffer[1];
	key = g_recvBuffer[1];
    
	for(i=2;i<8;i++)
	{
		val = g_recvBuffer[i];
        #if ENCPRYPT_ENABLE
		val^= key;
        #endif
        g_recvBuffer[i] = val;
		sum += val;               
	}

#if 0	
        printf("[%s,%d]",__FUNCTION__,__LINE__); 
        printf("2 g_recvBuffer:");
		for(i=2;i<8;i++)
		{
			printf("%02x ",g_recvBuffer[i]);
		}
        printf("\n");             
#endif	

    ucSum = (unsigned char)sum;
	if(ucSum == g_recvBuffer[i])
	{
        g_protocoldata.type = 0xAA; 
        memcpy(&g_protocoldata,g_recvBuffer+2,sizeof(g_protocoldata));
        usDeviceIDTemp = g_protocoldata.pair_key;

        g_usDeviceIDTempRev = usDeviceIDTemp;
        if(((usDeviceIDTemp == 0x0000)||(usDeviceIDTemp == 0xFFFF))&&(g_cMode == 0x02))//米㊣～∩?邦??豕??????㏒那?㏒?谷豕㊣???豕??????㏒那?
		{ 
            printf("g_usDevceID %02x\n",g_usDevceID );
            g_protocoldata.pair_key = g_usDevceID;
		   g_protocoldata.cmd_type = 0x82; 
           
            printf("enter match mode %02x \n",g_protocoldata.pair_key);
		}else{                        
            if(usDeviceIDTemp != g_usDevceID){
               printf("[%s,%d]Match Fails! host:%02x client:%02x \n",__FUNCTION__,__LINE__,
                g_usDevceID,usDeviceIDTemp);    
               memset(&g_protocoldata,0,sizeof(g_protocoldata)); //????﹞米???o∩?   
               ucMatchFlag  = 0;                                 //2?﹞米??
            }else{
               //printf("[%s,%d] ID:%02x Match OK!\n",__FUNCTION__,__LINE__,g_usDevceID); 
               ucMatchFlag  = 1;             
            }
		   g_protocoldata.cmd_type = 0x81; 
		}
		return 0;
	}
	memset(&g_protocoldata,0x00,sizeof(g_protocoldata));
	memset(g_recvBuffer,0x00,20);
	g_recvbufferlen = 0;
	
	return 0xFF;	
}


static u8 Is_LT_RecvData(void)
{
	unsigned short ret = 0;
    
	ret = LT_readreg(48);
	if(ret & FIFO_FLAG)
	{
		LT_RxDataEnable();
	}
	if(ret & PKT_FLAG) return 1;	//printf("PKT_FLAG == 1\r\n");		
	
   return 0;
}


static void LT_hwcfg(void)
{	
	//spi1 io init.
	cdr_system("himm 0x200f0050  0x01");//sclk spi1
	cdr_system("himm 0x200f0054  0x01");//spi1 sdo
	cdr_system("himm 0x200f0058  0x01");//spi1 sdi
	cdr_system("himm 0x200f005C  0x01");//spi1_csn0 
	
	cdr_system("himm 0x200f0074  0x0");//lt8900 Reset io
	//cdr_system("himm 0x20140400 0x02");
	cdr_set_gpio_mode(0x20140400,1,1);//GPIO 0-1 output
	cdr_system("himm 0x20140008 0x00");
	sleep(1);
    cdr_system("himm 0x20140008 0x02");
    sleep(1);   
}

#if(0)
static int LT_init(void)
{

    printf("[%s,%d] reg0:%04x, reg1:%04x\n",__FUNCTION__,__LINE__,
        LT_readreg(0),LT_readreg(1));   
    if(0x6FE0 != LT_readreg(0))
    {
      printf("write fails\n");
      return -1;
    }
 
    LT_writereg(0, 0x6F, 0xE0);
	LT_writereg(1, 0x56, 0x81);
	LT_writereg(2, 0x66, 0x17);
	LT_writereg(4, 0x9C, 0xC9);
	LT_writereg(5, 0x66, 0x37);
	LT_writereg(7, 0x00, 0x00);// use for setting 	RF frequency and to start or stop TX /RX packets  
	LT_writereg(8, 0x6C, 0x90);
	LT_writereg(9, 0x48, 0x00);//set TX power level 
	LT_writereg(10, 0x7F, 0xFD);//crystal osc.enabled 
	LT_writereg(11, 0x00, 0x08);//rssi enabled 
	LT_writereg(12, 0x00, 0x00);
	LT_writereg(13, 0x48, 0xBD);

	LT_writereg(22, 0x00, 0xff);
	LT_writereg(23, 0x80, 0x05);
	LT_writereg(24, 0x00, 0x67);
	LT_writereg(25, 0x16, 0x59);
	LT_writereg(26, 0x19, 0xE0);
	LT_writereg(27, 0x13, 0x00);
	LT_writereg(28, 0x18, 0x00);

	LT_writereg(32, 0x58, 0x00);//set preamble_len :3 byes.  set syncword_len:16 bits ----Reg36[15:0]
	LT_writereg(33, 0x3f, 0xC7);
	LT_writereg(34, 0x20, 0x00);
	LT_writereg(35, 0x03, 0x00);
	LT_writereg(36, LT_SYNCWORD[0], LT_SYNCWORD[1]);//set sync words 
	LT_writereg(37, LT_SYNCWORD[2], LT_SYNCWORD[3]);//set sync words 
	LT_writereg(38, LT_SYNCWORD[4], LT_SYNCWORD[5]);//set sync words 
	LT_writereg(39, LT_SYNCWORD[6], LT_SYNCWORD[7]);//set sync words 
	LT_writereg(40, 0x44, 0x01);
	LT_writereg(41, 0xb0, 0x00);//crc on scramble off ,1st byte packet length ,auto ack  off  
	LT_writereg(42, 0xFD, 0xB0);
	LT_writereg(43, 0x00, 0x0F);//configure scan_rssi

	//LT_writereg(44, 0x10, 0x00);
	//LT_writereg(45, 0x05, 0x52);	//62.5k	 

	LT_writereg(50, 0x00, 0x00);
	
	LT_Delayms(1000);   
    printf("LT8900 Init OK\r\n");
   return 0;
}
#else
static int LT_init(void)
{
#if LT8920_DEBUG
	printf("reg 0:%04x\r\n",LT_readreg(0));	
	printf("reg 1:%04x\r\n",LT_readreg(1));
#endif
	static int startflag = 0;
	startflag = 0;
	//LT_writereg(32, 0x58, 0x00);//1M  //set preamble_len :3 byes.  set syncword_len:16 bits ----Reg36[15:0]
    LT_writereg(32, 0x48, 0x00);//62.5  //set preamble_len :3 byes.  set syncword_len:16 bits ----Reg36[15:0]
        
	LT_writereg(33, 0x3f, 0xC7);
	LT_writereg(34, 0x20, 0x00);
	LT_writereg(35, 0x03, 0x00);
	LT_writereg(36, LT_SYNCWORD[0], LT_SYNCWORD[1]);//set sync words 
	LT_writereg(37, LT_SYNCWORD[2], LT_SYNCWORD[3]);//set sync words 
	LT_writereg(38, LT_SYNCWORD[4], LT_SYNCWORD[5]);//set sync words 
	LT_writereg(39, LT_SYNCWORD[6], LT_SYNCWORD[7]);//set sync words 
	LT_writereg(40, 0x44, 0x01);
	LT_writereg(41, 0xb0, 0x00);//crc on scramble off ,1st byte packet length ,auto ack  off  
	LT_writereg(42, 0xFD, 0xB0);
	LT_writereg(43, 0x00, 0x0F);//configure scan_rssi

	LT_writereg(44, 0x10, 0x00);
	LT_writereg(45, 0x05, 0x52);	//62.5k	 

    LT_writereg(0, 0x6F, 0xE0);
	LT_writereg(1, 0x56, 0x81);
	LT_writereg(2, 0x66, 0x17);
	LT_writereg(4, 0x9C, 0xC9);
	LT_writereg(5, 0x66, 0x37);
	LT_writereg(7, 0x00, 0x00);// use for setting 	RF frequency and to start or stop TX /RX packets  
	LT_writereg(8, 0x6C, 0x90);
	LT_writereg(9, 0x48, 0x00);//set TX power level 
	LT_writereg(10, 0x7F, 0xFD);//crystal osc.enabled 
	LT_writereg(11, 0x00, 0x08);//rssi enabled 
	LT_writereg(12, 0x00, 0x00);
	LT_writereg(13, 0x48, 0xBD);

	LT_writereg(22, 0x00, 0xff);
	LT_writereg(23, 0x80, 0x05);
	LT_writereg(24, 0x00, 0x67);
	LT_writereg(25, 0x16, 0x59);
	LT_writereg(26, 0x19, 0xE0);
	LT_writereg(27, 0x13, 0x00);
	LT_writereg(28, 0x18, 0x00);


	LT_writereg(50, 0x00, 0x00);


    printf("reg 0:%04x\r\n",LT_readreg(0));	
	printf("reg 1:%04x\r\n",LT_readreg(1));

    while(LT_readreg(32)!=0x4800 && startflag<5)
    {
	  	LT_writereg(32, 0x48, 0x00);//62.5  //set preamble_len :3 byes.  set syncword_len:16 bits ----Reg36[15:0]        
		LT_writereg(33, 0x3f, 0xC7);
		LT_writereg(34, 0x20, 0x00);
		LT_writereg(35, 0x03, 0x00);
		LT_writereg(36, LT_SYNCWORD[0], LT_SYNCWORD[1]);//set sync words 
		LT_writereg(37, LT_SYNCWORD[2], LT_SYNCWORD[3]);//set sync words 
		LT_writereg(38, LT_SYNCWORD[4], LT_SYNCWORD[5]);//set sync words 
		LT_writereg(39, LT_SYNCWORD[6], LT_SYNCWORD[7]);//set sync words 
		LT_writereg(40, 0x44, 0x01);
		LT_writereg(41, 0xb0, 0x00);//crc on scramble off ,1st byte packet length ,auto ack  off  
		LT_writereg(42, 0xFD, 0xB0);
		LT_writereg(43, 0x00, 0x0F);//configure scan_rssi

		LT_writereg(44, 0x10, 0x00);
		LT_writereg(45, 0x05, 0x52);	//62.5k	 

	    LT_writereg(0, 0x6F, 0xE0);
		LT_writereg(1, 0x56, 0x81);
		LT_writereg(2, 0x66, 0x17);
		LT_writereg(4, 0x9C, 0xC9);
		LT_writereg(5, 0x66, 0x37);
		LT_writereg(7, 0x00, 0x00);// use for setting 	RF frequency and to start or stop TX /RX packets  
		LT_writereg(8, 0x6C, 0x90);
		LT_writereg(9, 0x48, 0x00);//set TX power level 
		LT_writereg(10, 0x7F, 0xFD);//crystal osc.enabled 
		LT_writereg(11, 0x00, 0x08);//rssi enabled 
		LT_writereg(12, 0x00, 0x00);
		LT_writereg(13, 0x48, 0xBD);

		LT_writereg(22, 0x00, 0xff);
		LT_writereg(23, 0x80, 0x05);
		LT_writereg(24, 0x00, 0x67);
		LT_writereg(25, 0x16, 0x59);
		LT_writereg(26, 0x19, 0xE0);
		LT_writereg(27, 0x13, 0x00);
		LT_writereg(28, 0x18, 0x00);

		LT_writereg(50, 0x00, 0x00);
		startflag++;
		usleep(100000);
	             
    }    

    LT_Delayms(1000);
    
#if 1//LT8920_DEBUG    
    printf("LT8900 Init OK .\r\n");
#endif

    return 0;

}
#endif

static void LT_PackAndSendData(void) 
{ 	
	LT_Snd_Data((unsigned char *)&g_protocoldata,ACK_BUFFER_LEN); 
} 

static void start_lt8900_handle_thread(void) 
{ 	
	pthread_t tfid;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, (void *)cdr_lt8900_handle_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);
}

static int cdr_lt8900_getmac_value()
{ 
    struct   ifreq   ifreq; 
    int   sock; 
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
        return   2; 
    } 
    strcpy(ifreq.ifr_name,"wlan0"); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    {       
        return   3; 
    } 

    int iMac = (((unsigned short)ifreq.ifr_hwaddr.sa_data[4])<<8)|ifreq.ifr_hwaddr.sa_data[5];

    return   iMac; 
}

int cdr_lt8900_init(unsigned short usDeviceID)
{
	int nRet = 0;
		
	LT_hwcfg();
    
	nRet = LT_init();
    if(nRet == -1)
    {
		printf("[%s %d] LT_init fails\n",__FUNCTION__,__LINE__);
		return -1;
    }
    
    g_usDevceID = cdr_lt8900_getmac_value();

    printf("g_usDevceID %02x\n",g_usDevceID );
	start_lt8900_handle_thread();

	return 1;
}


void cdr_lt8900_setevent_callbak(cdr_lt8900_callback callback)
{
	if(callback == NULL)
	{
		//printf("callbak is NULL\n");
		return;
	}
    
	g_lt8900_fun_callbak = callback;

    if(g_lt8900_fun_callbak != NULL)	
    {
		printf("%s register g_lt8900_fun_callbak .\r\n",__FUNCTION__);
    }
}

#if 0

unsigned char ucRepeatFlag = 0;

int cdr_lt8900_handle_thread()
{
    int LT_RxCnt = 0;  
    int iKeyValue = 0;      

    srand((int)time(0));  
    
    LT_RxDataEnable();

    cdr_lt8900_callback _fcallbak = g_lt8900_fun_callbak;

    int timep1,timep2,timep3;
    
    while(1)
	{	
        _fcallbak = g_lt8900_fun_callbak;        
        //sleep(1);
        usleep(100000);
        if(Is_LT_RecvData())//那?﹞?那?米?那y?Y 
        { 			
        	if(0 == LT_RecvData())
        	{
                 if((ucMatchFlag  == 1)||(g_cMode == 0x02))
                 {                      
        				iKeyValue = g_recvBuffer[6];

        				if((g_protocoldata.cmd_type == 0x81)&&_fcallbak)
        				{
                          if(ucRepeatFlag == 0x00)
                          {
                                timep1 = time((time_t*)NULL);
                                //printf("first time : %d \n",timep1);
                                 _fcallbak(iKeyValue);
                                 ucRepeatFlag++;
                                 LT_RxDataEnable();
                                 continue;
                          }
                          if(ucRepeatFlag == 0x01)
                          {                                
                                timep2 = time((time_t*)NULL);
                                //printf("time second: %d \n",timep2 - timep1);
                                if((timep2 - timep1) > 1)
                                {
                					_fcallbak(iKeyValue);	
                                   ucRepeatFlag++;
                                   LT_RxDataEnable();
                                   continue;
                                }
                                
                          } 
                          if(ucRepeatFlag == 0x02)
                          {
                                timep3 = time((time_t*)NULL);
                                //printf("time second: %d \n",timep2 - timep1);
                                if((timep3 - timep2) > 1)
                                {
                					_fcallbak(iKeyValue);	
                                   ucRepeatFlag = 0;
                                   LT_RxDataEnable();
                                   continue;
                                }
                          }
        				}

                      LT_Delayms(50);			                
        				LT_PackAndSendData();//﹞⊿?赤∩e車|那y?Y 				                        
        				LT_Delayms(100);			                
        				LT_PackAndSendData();//﹞⊿?赤∩e車|那y?Y 				        				
                        
                 }
        	}	
        	LT_RxDataEnable();
        } 
	}
}



#endif

#define INTERVAL_TIME  1
#if(1)
unsigned char ucRepeatFlag = 0;

int cdr_lt8900_handle_thread()
{
    //int LT_RxCnt = 0;  
    int iKeyValue = 0;      

    srand((int)time(0));  
    
    LT_RxDataEnable();

    cdr_lt8900_callback _fcallbak = g_lt8900_fun_callbak;

    static int timep1 = 0,timep2 = 0,timep3 = 0;
    
    while(1)
	{	
        _fcallbak = g_lt8900_fun_callbak;        
         usleep(1000);
        if(Is_LT_RecvData())
        { 			
        	if(0 == LT_RecvData())
        	{
                 if((ucMatchFlag  == 0x01)||(g_cMode == 0x02))
                 {                      
        				iKeyValue = g_recvBuffer[6];

        				if((g_protocoldata.cmd_type == 0x81)&&_fcallbak &&(g_usDevceID == g_usDeviceIDTempRev))
        				{
                          if(ucRepeatFlag == 0x00)
                          {
                                timep1 = time((time_t*)NULL);
                                //printf("first time : %d \n",timep1);
                                 _fcallbak(iKeyValue);
                                 ucRepeatFlag++;
                                 LT_RxDataEnable();
                                 //continue;
                          }
                          else if(ucRepeatFlag == 0x01)
                          {                                
                                timep2 = time((time_t*)NULL);
                                //printf("time second: %d \n",timep2 - timep1);
                                ucRepeatFlag++;
                                if((timep2 - timep1) > INTERVAL_TIME)
                                {
                					_fcallbak(iKeyValue);	
                                   //ucRepeatFlag++;
                                   LT_RxDataEnable();                                   
                                }else{
                                   continue;
                                }
                                
                          } 
                          else if(ucRepeatFlag == 0x02)
                          {
                                timep3 = time((time_t*)NULL);
                                //printf("time second: %d \n",timep2 - timep1);
                                ucRepeatFlag = 0;
                                
                                if((timep3 - timep2) > INTERVAL_TIME)
                                {
                					_fcallbak(iKeyValue);	
                                   ucRepeatFlag = 0;
                                   LT_RxDataEnable();
                                   //continue;
                                }else{
                                   continue;
                                }
                          }
        				}

                      LT_Delayms(50);			                
        				LT_PackAndSendData();//﹞⊿?赤∩e車|那y?Y 				                        
        				LT_Delayms(50);			                
        				LT_PackAndSendData();//﹞⊿?赤∩e車|那y?Y 				        				
                        
                 }
        	}	
        	LT_RxDataEnable();
        } 
	}
}
#endif

static void cdr_lt8900_open(void)
{
    cdr_system("himm 0x200f005C  0x01");//spi1_csn0 ???? io   ?∩車?谷豕??      
    cdr_system("himm 0x20140008 0x00"); //????g0_1 io那y?Y??2迄℅¯ ??米赤 ??豕?2??邦?車那??㏒那?
    sleep(1);
    cdr_system("himm 0x20140008 0x02"); //?-那??米 g0_1 ???? ??豕?1∟℅¯足?   
    sleep(1);
    LT_init();
}

void cdr_lt8900_close(void)
{
    LT_writereg(7, 0x00, 0x80);
    LT_writereg(7, 0x00, 0x00); //???D
    LT_writereg(35, 0x40, 0x00);//sleep close ?∫??
    cdr_system("himm 0x200f005C  0x00");//spi1_csn0 ???? io  disable ?∩車?谷豕??  
    cdr_system("himm 0x20140008 0x00"); //reset
}

/*
*  D-辰谷 00 close 01 open  02 match 03 umatch
*  ㊣?米? 00 --    01 match 02 close 03 open 
*/
void  cdr_lt8900_mode_ctrl(eLt8900DeviceMode ucMode)
{
    switch(ucMode){     
      case LT8900_MODE_CLOSE:    
        g_cMode = 0x00;       
        break;
      case LT8900_MODE_OPEN:      
        g_cMode = 0x01;      
        break;
      case LT8900_MATCH_MODE_ENABLE:   
        g_cMode = 0x02;     
        break;
      default:  return;
    }
    switch(g_cMode){
      case 0x00: 
        cdr_lt8900_close();      
        break;
      case 0x01:  
        cdr_lt8900_open();       
        break;
      case 0x02:  
        signal(SIGALRM,reset_match_mode);   
        alarm(30);
        break;     
      default:break;    

   }  
}

int cdr_lt8900_factory_test()
{
	LT_hwcfg();
    if(0x6FE0 != LT_readreg(0))
    {
      printf("write fails\n");
      return -1;
    }
	return 0;	
}

/*30s auto exit match mode*/
static void reset_match_mode(int sig)  
{  
    if(SIGALRM == sig)  
    {  
        g_cMode = 0x03;
        printf("[%s %d]LT8900 pair mode end.\n",__FUNCTION__,__LINE__);         
    }    
    return ;  
}  


