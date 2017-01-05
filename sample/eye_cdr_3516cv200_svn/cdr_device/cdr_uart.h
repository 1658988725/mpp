/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/

#ifndef     UART_H
#define     UART_H


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif 

int cdr_uart_init(void);

typedef int (*cdr_uart_callback)(char *pRevcBuff);

void cdr_uart_setevent_callbak(cdr_uart_callback callback);

int uart_get_dst_bag(char *pSrcBuf,char *pDstBag);

int uart_rev(void);
extern unsigned char g_ucGpsDataBuff[300];


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif 

#endif


