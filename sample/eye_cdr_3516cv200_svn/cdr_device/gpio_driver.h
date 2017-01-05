#ifndef _GPIO_DRIVER_H
#define _GPIO_DRIVER_H

struct DRV_gpio_ioctl_data {
    unsigned int uRegAddr;
    unsigned int uRegValue;
};

#define DRV_gpio_MAGIC      'x'		//定义幻数
#define DRV_gpio_MAX_NR		6	//定义命令的最大序数

#define DRV_gpioSetMode     _IO(DRV_gpio_MAGIC, 1)
#define DRV_gpioSet         _IO(DRV_gpio_MAGIC, 2)
#define DRV_gpioClr         _IO(DRV_gpio_MAGIC, 3)
#define DRV_gpioGet         _IO(DRV_gpio_MAGIC, 4)
#define DRV_reg_read        _IO(DRV_gpio_MAGIC, 5)
#define DRV_reg_write       _IO(DRV_gpio_MAGIC, 6)

#endif

