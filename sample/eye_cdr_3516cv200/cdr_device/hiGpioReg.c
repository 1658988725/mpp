#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
//#include <gpio_driver.h>
#include "gpio_driver.h"
//#include "gos_log.h"

#if(0)

static int dev_open()
{
    int fd = open("/dev/gpioDriver", 0);
    if (fd < 0) 
    {
        printf("Can't open /dev/gpioDriver\n");
        return -1;
    }
    return fd;
}

static int dev_close(int fd)
{
    if(fd)
        close(fd);
    
    return 0;
}


int gpioSetMode(unsigned char gpioBank, unsigned char gpioBit
    , unsigned char gpioDir, unsigned char gpioValue)
{
    int ret, gpiofd;
    unsigned long arg = 0;
    
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;

    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
    arg |= gpioDir<<8;
    arg |= gpioValue;
    
    
    ret = ioctl(gpiofd, DRV_gpioSetMode, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioSet(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret, gpiofd;
    unsigned long arg = 0;
    
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;

    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioSet, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioClr(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret, gpiofd;
    unsigned long arg = 0;
    
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;

    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioClr, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioGet(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret, gpiofd;
    unsigned long arg = 0;
    
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;

    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioGet, arg);

    dev_close(gpiofd);
    return ret;
}

int reg_read(unsigned int arg, unsigned int *regvalue)
{
    int gpiofd;
    
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;

    *regvalue = ioctl(gpiofd, DRV_reg_read, arg);

    dev_close(gpiofd);
    return 0;
}

int reg_write(unsigned int arg, unsigned int regvalue)
{
    int ret, gpiofd;
    
    gpiofd = dev_open();//这个地方若重复调用，次数过频繁，则存在问题，可修成static 模式
    if(gpiofd < 0)
        return -1;
    struct DRV_gpio_ioctl_data val;
    val.uRegAddr = arg;
    val.uRegValue = regvalue;
    ret = ioctl(gpiofd, DRV_reg_write, &val);

    dev_close(gpiofd);
    return ret;
}
#else

#if(0)
static int gpiofd;

static int dev_open()
{
    gpiofd = open("/dev/gpioDriver", 0);
    if (gpiofd < 0) 
    {
        printf("Can't open /dev/gpioDriver\n");
        return -1;
    }
    return gpiofd;
}

static int dev_close(int fd)
{
    if(gpiofd)
        close(gpiofd);
    
    return 0;
}


int gpioSetMode(unsigned char gpioBank, unsigned char gpioBit
    , unsigned char gpioDir, unsigned char gpioValue)
{
    int ret;
    unsigned long arg = 0;
#if 0     
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;
#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    
    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
    arg |= gpioDir<<8;
    arg |= gpioValue;
    
    
    ret = ioctl(gpiofd, DRV_gpioSetMode, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioSet(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret;
    unsigned long arg = 0;
#if 0     
    gpiofd = dev_open();
    if(gpiofd < 0)
        return -1;
#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioSet, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioClr(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret;
    unsigned long arg = 0;
#if 1     
    gpiofd = dev_open();

#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioClr, arg);

    dev_close(gpiofd);
    return ret;
}

int gpioGet(unsigned char gpioBank, unsigned char gpioBit)
{
    int ret;
    unsigned long arg = 0;
#if 1    
    gpiofd = dev_open();
    
#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    arg |= gpioBank<<24;
    arg |= gpioBit<<16;
       
    ret = ioctl(gpiofd, DRV_gpioGet, arg);

    dev_close(gpiofd);
    return ret;
}

int reg_read(unsigned int arg, unsigned int *regvalue)
{
    //int gpiofd;
 #if 1    
    gpiofd = dev_open();
    
#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    *regvalue = ioctl(gpiofd, DRV_reg_read, arg);

    dev_close(gpiofd);
    return 0;
}

int reg_write(unsigned int arg, unsigned int regvalue)
{
    int ret;
#if 1   
    gpiofd = dev_open();//这个地方若重复调用，次数过频繁，则存在问题，可修成static 模式
    
#endif
    if(gpiofd <= 0){
        printf("[%s %d] file not open yet!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    struct DRV_gpio_ioctl_data val;
    val.uRegAddr = arg;
    val.uRegValue = regvalue;
    ret = ioctl(gpiofd, DRV_reg_write, &val);

    dev_close(gpiofd);
    return ret;
}
#endif
#endif


