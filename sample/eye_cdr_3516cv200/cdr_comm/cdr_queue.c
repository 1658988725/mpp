/************************************************************************	
** Filename: 	cdr_comm.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include<stdio.h>  
#include<stdlib.h>  
#include"malloc.h"  
#include"cdr_queue.h"  
#include <string.h>

//串口接收队列

volatile Queue stUartQueue;

unsigned char ucUartQueueBuf[UART_QUEUE_BUF_SIZE];


//--------------------------------------------------------------
unsigned char QueueAddData(Queue *stQueue, unsigned char ucData)
{
    stQueue->pucBuf[stQueue->usTail] = ucData;

    if(stQueue->usTail == stQueue->usBufSize - 1)
    {
     stQueue->usTail = 0;
    }
    else
    {
     stQueue->usTail = stQueue->usTail+1;
    }
    return 1;
}

//--------------------------------------------------------------
//功能描述； 从stQueue数组中读取一个字节存入pData
//输入参数: stQueue-待提取数组
//输出参数: pData -存入的数组
//返回值 : 1 - 成功 0－失败
//--------------------------------------------------------------
unsigned char QueueGetData(Queue *stQueue,unsigned char *pData)
{
    if(stQueue->usHead == stQueue->usTail)
    {
     return 0;
    }

    *pData = stQueue->pucBuf[stQueue->usHead];

    if(stQueue->usHead == stQueue->usBufSize - 1)
    {
     stQueue->usHead = 0;
    }
    else
    {
     stQueue->usHead = stQueue->usHead + 1;
    }

   return 1;
}
//--------------------------------------------------------------
unsigned char QueueCheckEmpty(Queue *stQueue)
{
 if(stQueue->usHead == stQueue->usTail)
 {
  return 1;
 }
 return 0;
}
//--------------------------------------------------------------
void UartRecvQueueInit(Queue *stQueue)
{
 stQueue->pucBuf = &ucUartQueueBuf[0];
 stQueue->usHead = 0;
 stQueue->usTail = 0;
 stQueue->usBufSize = UART_QUEUE_BUF_SIZE;
 memset(stQueue->pucBuf, 0, stQueue->usBufSize);
}

