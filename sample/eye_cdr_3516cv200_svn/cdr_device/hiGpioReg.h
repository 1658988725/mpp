/******************************************************************************

                  版权所有 (C), 2012-2022, Goscam

 ******************************************************************************
  文 件 名   : hiGpioReg.h
  版 本 号   : 初稿
  作    者   : mr.iubing
  生成日期   : 2014年3月25日
  最近修改   :
  功能描述   : hiGpioReg.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2014年3月25日
    作    者   : mr.iubing
    修改内容   : 创建文件

******************************************************************************/

#ifndef __HIGPIOREG_H__
#define __HIGPIOREG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define GPIO_INPUT     0
#define GPIO_OUTPUT    1

extern int gpioClr(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioGet(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioSet(unsigned char gpioBank, unsigned char gpioBit);
extern int gpioSetMode(unsigned char gpioBank, unsigned char gpioBit
             , unsigned char gpioDir, unsigned char gpioValue);
extern int reg_read(unsigned int arg, unsigned int *regvalue);
extern int reg_write(unsigned int arg, unsigned int regvalue);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HIGPIOREG_H__ */
