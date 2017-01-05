/************************************************************************
//Stream_pool.h
//a pool for vedio&audio 
//1.encode thread write stream buffer to pool
//2.Record thread read stream buffer from pool write to tf
//3.manage buffer.
*************************************************************************/
#ifndef _STREAM_POOL_H
#define _STREAM_POOL_H

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
#include "cdr_comm.h"

#define VSPOOLMAXSIZE     150	    //30 * 5s (per sec 30frame)
#define POOLSIZE          150+1
#define MAX_VIDEODATA_TF  20485760 //10M
#define MAX_AUDIODATA_TF  1048576  //1M (1024*1024)

#define MAX_VSLIVE_POOL   2097152  //2*1024*1024  2M
#define MAX_ASLIVE_POOL   1048576  //1M

#define ENUM_STREM_VIEDO 0
#define ENUM_STREM_AUDIO 1
#define ENUM_STREM_OTHER 2
#define H264             4
#define AAC              5

enum ENUMASPool_Type
{
	ENUM_VS_REC   = 0,
	ENUM_AS_REC   = 1,
	ENUM_VS_LIVE  = 2,
	ENUM_AS_LIVE  = 3,
	ENUM_POOL_MAX = 4	
};

//Frame info
typedef struct _VideoData
{
    short IFrame;
    short iEnc;
    short StreamType;
    unsigned int  nSize;
    unsigned int  nPos;
}VideoData;

//Pool Struct
typedef struct{
    unsigned int g_readIndex;  //Read index,encode thread write index.
    unsigned int g_writeIndex; //Write to sd index.
    unsigned int g_bLost;	   //Need lost buffer.The sd write too slow
    unsigned int g_bAgain;     //Reset

    unsigned int g_writePos;   
    unsigned int g_readPos;
    unsigned int g_allDataSize;//All data leng
    unsigned int g_CurPos;

    unsigned int g_haveFrameCnt;
    char *g_videoBuf;			//buff.

    VideoData g_VideoData[POOLSIZE+1];
}VideoDataRecord;

//vaframe info
typedef struct{
	short isIframe;//帧类型而非I帧类型
	short iEnc;
	short iStreamtype;//video / audio
	unsigned long long u64PTS;	
}sVAFrameInfo;

//Frame info
typedef struct _VAItemInfo
{
	sVAFrameInfo 	oVAFrame;
    unsigned int  nSize;
    unsigned int 	nPos;
}VAItemInfo;

//Pool Struct
typedef struct _VA_pool
{
    unsigned int g_readIndex;  //Read index,encode thread write index.
    unsigned int g_writeIndex; //Write to sd index.
    unsigned int g_bLost;	    //Need lost buffer.The sd write too slow
    unsigned int g_bAgain;     //Reset
    unsigned int g_writePos;   //
    unsigned int g_readPos;
    unsigned int g_allDataSize;//All data leng
    unsigned int g_CurPos;	
    unsigned int g_haveFrameCnt;	
	
	unsigned int g_oBufMaxLen; //len of g_vaBuf	
	unsigned int g_poolSize;	
    
	pthread_mutex_t g_threadmutex;
    char *g_vaBuf;			//buff.
    VAItemInfo g_oFramelist[VSPOOLMAXSIZE];
}VA_POOL;

VA_POOL *VAPool_colloc(unsigned int nMaxFrameSize,unsigned int nBufferSize);
void VAPool_free(VA_POOL *pVAPool);
void VAPool_InitOthers(VA_POOL *pVAPool);

int StremPoolWrite(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame);
int StremPoolRead(int pStreamType,char** pData,unsigned int* nSize,sVAFrameInfo *vaFrame);

#endif //_STREAM_POOL_H
