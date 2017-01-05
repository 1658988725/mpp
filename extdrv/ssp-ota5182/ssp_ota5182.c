/*  extdrv/interface/ssp/hi_ssp.c
 *
 * Copyright (c) 2006 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 * History:
 *      21-April-2006 create this file
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>


#include "hi_ssp.h"

#define  ssp_readw(addr,ret)			(ret =(*(volatile unsigned int *)(addr)))
#define  ssp_writew(addr,value)			((*(volatile unsigned int *)(addr)) = (value))

#define  HI_REG_READ(addr,ret)			(ret =(*(volatile unsigned int *)(addr)))
#define  HI_REG_WRITE(addr,value)		((*(volatile unsigned int *)(addr)) = (value))

#define SSP_BASE	0x200E0000
#define SSP_SIZE	0x10000				// 64KB
#define SSP_INT		65					// Interrupt No.

void __iomem* reg_ssp_base_va;
#define IO_ADDRESS_VERIFY(x) (reg_ssp_base_va + ((x)-(SSP_BASE)))

/* SSP register definition .*/
#define SSP_CR0              IO_ADDRESS_VERIFY(SSP_BASE + 0x00)
#define SSP_CR1              IO_ADDRESS_VERIFY(SSP_BASE + 0x04)
#define SSP_DR               IO_ADDRESS_VERIFY(SSP_BASE + 0x08)
#define SSP_SR               IO_ADDRESS_VERIFY(SSP_BASE + 0x0C)
#define SSP_CPSR             IO_ADDRESS_VERIFY(SSP_BASE + 0x10)
#define SSP_IMSC             IO_ADDRESS_VERIFY(SSP_BASE + 0x14)
#define SSP_RIS              IO_ADDRESS_VERIFY(SSP_BASE + 0x18)
#define SSP_MIS              IO_ADDRESS_VERIFY(SSP_BASE + 0x1C)
#define SSP_ICR              IO_ADDRESS_VERIFY(SSP_BASE + 0x20)
#define SSP_DMACR            IO_ADDRESS_VERIFY(SSP_BASE + 0x24)




void hi_ssp_writeOnly(int bWriteOnly)
{
    int ret = 0;

    bWriteOnly = 0;

    ssp_readw(SSP_CR1, ret);

    if (bWriteOnly)
    {
        ret = ret | (0x1 << 5);
    }
    else
    {
        ret = ret & (~(0x1 << 5));
    }

    ssp_writew(SSP_CR1, ret);
}


void hi_ssp_enable(void)
{
    int ret = 0;
    ssp_readw(SSP_CR1, ret);
    ret = (ret & 0xFFFD) | 0x2;

    ret = ret | (0x1 << 4); /* big/little end, 1: little, 0: big */

    ret = ret | (0x1 << 15); /* wait en */

    ssp_writew(SSP_CR1, ret);

    hi_ssp_writeOnly(0);
}


void hi_ssp_disable(void)
{
    int ret = 0;
    ssp_readw(SSP_CR1, ret);
    ret = ret & (~(0x1 << 1));
    ssp_writew(SSP_CR1, ret);
}

int hi_ssp_set_frameform(unsigned char framemode, unsigned char spo, unsigned char sph, unsigned char datawidth)
{
    int ret = 0;
    ssp_readw(SSP_CR0, ret);
    if (framemode > 3)
    {
        printk("set frame parameter err.\n");
        return -1;
    }
    ret = (ret & 0xFFCF) | (framemode << 4);
    if ((ret & 0x30) == 0)
    {
        if (spo > 1)
        {
            printk("set spo parameter err.\n");
            return -1;
        }
        if (sph > 1)
        {
            printk("set sph parameter err.\n");
            return -1;
        }
        ret = (ret & 0xFF3F) | (sph << 7) | (spo << 6);
    }
    if ((datawidth > 16) || (datawidth < 4))
    {
        printk("set datawidth parameter err.\n");
        return -1;
    }
    ret = (ret & 0xFFF0) | (datawidth - 1);
    ssp_writew(SSP_CR0, ret);
    return 0;
}


int hi_ssp_set_serialclock(unsigned char scr, unsigned char cpsdvsr)
{
    int ret = 0;
    ssp_readw(SSP_CR0, ret);
    ret = (ret & 0xFF) | (scr << 8);
    ssp_writew(SSP_CR0, ret);
    if ((cpsdvsr & 0x1))
    {
        printk("set cpsdvsr parameter err.\n");
        return -1;
    }
    ssp_writew(SSP_CPSR, cpsdvsr);
    return 0;
}

int hi_ssp_alt_mode_set(int enable)
{
    int ret = 0;

    ssp_readw(SSP_CR1, ret);
    if (enable)
    {
        ret = ret & (~0x40);
    }
    else
    {
        ret = (ret & 0xFF) | 0x40;
    }
    ssp_writew(SSP_CR1, ret);

    return 0;
}


void hi_ssp_writedata(unsigned short sdata)
{
    ssp_writew(SSP_DR, sdata);
    udelay(2);
}

int hi_ssp_readdata(void)
{
    int ret = 0;
    ssp_readw(SSP_DR, ret);
    return ret;
}


static void spi_enable(void)
{
    HI_REG_WRITE(SSP_CR1, 0x42);
}

static void spi_disable(void)
{
    HI_REG_WRITE(SSP_CR1, 0x40);
}


int hi_ssp_lcd_init_cfg(void)
{
    unsigned char framemode = 0;
    unsigned char spo = 1;
    unsigned char sph = 1;
    unsigned char datawidth = 9;

#ifdef HI_FPGA
    unsigned char scr = 1;
    unsigned char cpsdvsr = 2;
#else
    unsigned char scr = 8;
    unsigned char cpsdvsr = 8;
#endif

    spi_disable();

    hi_ssp_set_frameform(framemode, spo, sph, datawidth);

    hi_ssp_set_serialclock(scr, cpsdvsr);

    hi_ssp_alt_mode_set(1);

    hi_ssp_enable();

    return 0;
}

void spi_write_a9byte(unsigned char cmd_dat, unsigned char dat)
{
    unsigned short spi_data = 0;

    if (cmd_dat)
    {
        spi_data = 1 << 8;
    }
    else
    {
        spi_data = 0 << 8;
    }

    spi_data = spi_data | dat;
    spi_enable();
    //hi_ssp_writedata(spi_data);
    ssp_writew(SSP_DR, spi_data);
    printk("spi_data:0x%x\n", spi_data);
    msleep(10);
    spi_disable();
}

void spi_write_a16byte(unsigned short spi_dat)
{
    spi_enable();
    //hi_ssp_writedata(spi_data);
    ssp_writew(SSP_DR, spi_dat);
    printk("spi_data:0x%x\n", spi_dat);
    msleep(10);
    spi_disable();
}

void ssp_write_dat(unsigned char dat)
{
    spi_write_a9byte(1, dat);
}

void ssp_write_cmd(unsigned char dat)
{
    spi_write_a9byte(0, dat);
}

long ssp_lcd_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    unsigned char val;
    unsigned short val_16;

    switch (cmd)
    {
        case SSP_LCD_READ_ALT:

            break;

        case SSP_LCD_WRITE_CMD:
            val = *(unsigned int*)arg;
            spi_write_a9byte(0, val);
            //printk("SSP_LCD_WRITE_CMD!\n");
            break;

        case SSP_LCD_WRITE_DAT:
            val = *(unsigned int*)arg;
            spi_write_a9byte(1, val);
            break;

        case SSP_LCD_WRITE_CMD16:
            val_16 = *(unsigned int*)arg;
            spi_write_a16byte(val_16);
            break;

        default:
        {
            printk("Kernel: No such ssp command %#x!\n", cmd);
            return -1;
        }
    }

    return 0;
}

int ssp_lcd_open(struct inode* inode, struct file* file)
{
    return 0;
}
int ssp_lcd_close(struct inode* inode, struct file* file)
{
    return 0;
}


static struct file_operations ssp_lcd_fops =
{
    .owner      = THIS_MODULE,
    .unlocked_ioctl = ssp_lcd_ioctl,
    .open       = ssp_lcd_open,
    .release    = ssp_lcd_close
};


static struct miscdevice ssp_lcd_dev =
{
    .minor		= MISC_DYNAMIC_MINOR,
    .name		= "ssp_5182",
    .fops  		= &ssp_lcd_fops,
};





void lcd_ota5182_init(void)
{
    /*spi_16bit_setting*/
    unsigned char framemode = 0;
    unsigned char spo = 1;
    unsigned char sph = 1;
    unsigned char datawidth = 16;
#ifdef HI_FPGA
    unsigned char scr = 1;
    unsigned char cpsdvsr = 2;
#else
    unsigned char scr = 8;
    unsigned char cpsdvsr = 8;
#endif
    spi_disable();
    hi_ssp_set_frameform(framemode, spo, sph, datawidth);
    hi_ssp_set_serialclock(scr, cpsdvsr);
    hi_ssp_alt_mode_set(1);
    hi_ssp_enable();

    /*LCD ILI9341 VERTICAL SETTING*/
    //ssp_write_cmd(0x01);//software reset

    spi_write_a16byte(0x000f);
    spi_write_a16byte(0x0005);
    msleep(10);
    spi_write_a16byte(0x000f);
    spi_write_a16byte(0x0005);
    msleep(10);
    spi_write_a16byte(0x000f);
    spi_write_a16byte(0x5000);
    msleep(10);
    //spi_write_a16byte(0x10E4);
    spi_write_a16byte(0x1004);
    spi_write_a16byte(0x3008);
    spi_write_a16byte(0x7040);//brightness
    spi_write_a16byte(0xA000);
    spi_write_a16byte(0xC003);
    spi_write_a16byte(0xE013);
    spi_write_a16byte(0x6001);
    msleep(100);
}



static int __init hi_ssp_lcd_init(void)
{
    int ret;

    reg_ssp_base_va = ioremap_nocache((unsigned long)SSP_BASE, (unsigned long)SSP_SIZE);
    if (!reg_ssp_base_va)
    {
        printk("Kernel: ioremap ssp base failed!\n");
        return -ENOMEM;
    }

    ret = misc_register(&ssp_lcd_dev);
    if (0 != ret)
    {
        printk("Kernel: register ssp_0 device failed!\n");
        return -1;
    }
    printk("lcd is 0ta5182\n");
    lcd_ota5182_init();
    printk("Kernel: ssp_lcd initial ok!\n");

    return 0;

}

static void __exit hi_ssp_lcd_exit(void)
{

    hi_ssp_disable();

    iounmap((void*)reg_ssp_base_va);

    misc_deregister(&ssp_lcd_dev);
}




module_init(hi_ssp_lcd_init);
module_exit(hi_ssp_lcd_exit);

MODULE_LICENSE("GPL");

