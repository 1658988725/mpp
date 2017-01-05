
#ifndef __QUEUE_H_  
#define __QUEUE_H_  

//#define UART_QUEUE_BUF_SIZE 592+100 //74 * 8
#define UART_QUEUE_BUF_SIZE 1024 //74 * 8

typedef struct
{
 unsigned char *pucBuf;
 unsigned short usHead;
 unsigned short usTail;
 unsigned short usBufSize;
}Queue;

extern volatile Queue stUartQueue;

void UartRecvQueueInit(Queue *stQueue);
unsigned char QueueAddData(Queue *stQueue, unsigned char ucData);
unsigned char QueueGetData(Queue *stQueue, unsigned char *pData);
unsigned char QueueCheckEmpty(Queue *stQueue);
unsigned short QueueGetCount(Queue *stQueue);

#endif  

