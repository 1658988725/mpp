#ifndef _SHM_COM_H
#define _SHM_COM_H

#include <unistd.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <sys/ipc.h>  
#include <sys/shm.h>  
#include <sys/sem.h>


#define MMAPPRINT_ENABLE    0
#if MMAPPRINT_ENABLE
    #define MMAPDEBUG_PRT(fmt...)  \
        do {                 \
            printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
            printf(fmt);     \
        }while(0)
#else
    #define MMAPDEBUG_PRT(fmt...)  \
        do { ; } while(0)
#endif



#define MAX_VIDEO_BUFFSIZE 10485760
#define MAX_AUDIO_BUFFSIZE 1048576

#define POOLSIZE 150+1

#define VIDIOSEM_KEYID  0x1233
#define VIDEOPOOL_KEYID  0x1234 
#define VIDEOBUF_KEYID  0x1235
  
#define AUDIOPOOL_KEYID  0x1236
#define AUDIOBUF_KEYID  0x1237


#define ENUM_STREM_VIEDO 0
#define ENUM_STREM_AUDIO 1
#define ENUM_STREM_OTHER 2

 
 //Frame info
 typedef struct _DataItem
 {
	 short IFrame;
	 short iEnc;
	 short StreamType;
	 unsigned int  nSize;
	 unsigned int  nPos;
 }DATAITEM;
 
 //Pool Struct
typedef struct _shared_use_st
{
    unsigned int g_readIndex;  //Read index,encode thread write index.
    unsigned int g_writeIndex; //Write to sd index.
    unsigned int g_bLost;	   //Need lost buffer.The sd write too slow
    unsigned int g_bAgain;     //Reset
    unsigned int g_writePos;   //
    unsigned int g_readPos;
    unsigned int g_allDataSize;//All data leng
    unsigned int g_CurPos;
    unsigned int g_haveFrameCnt;
    char g_Buf[MAX_VIDEO_BUFFSIZE];//buff.
    DATAITEM g_item[POOLSIZE+1];
}SHARED_USE_ST;


 //联合类型semun定义  
 union semun{  
	 int val;  
	 struct semid_ds *buf;	
	 unsigned short *array;  
 };  


 //Init the stream pool 
int MMAPPoolInit(int pStreamType);

//Reset the stream pool 
int MMAPPoolReset(int pStreamType);

//Realase the stream pool 
int MMAPPoolRealase(int pStreamType);

int MAAPWriteVedioData(int pStreamType,char* pData,unsigned int nSize,int IFrame,int iResType, int iStreamType);

int MAAPReadVedioData(int pStreamType,char** pData,unsigned int* nSize,int *IFrame,int *iResType, int *iStreamType);

#endif
