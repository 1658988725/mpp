#include "stream_pool.h"
#include <pthread.h>


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

static VA_POOL *g_oPoolList[10];


static pthread_mutex_t g_MutexLock_STREAM_POOL = PTHREAD_MUTEX_INITIALIZER;


static int VAPool_Write(VA_POOL *pVAPool,char* pData,unsigned int nSize,sVAFrameInfo vaFrame);
static int VAPool_Read(VA_POOL *pVAPool,char** pData,unsigned int* nSize,sVAFrameInfo *vaFrame);


//static int WriteDataToPool(int pStreamType,char* pData,unsigned int nSize,int IFrame,int iResType, int iStreamType, VideoDataRecord *mPool);
//static int ReadDataFromPool(int pStreamType,char** pData,unsigned int* nSize,int *IFrame,int *iResType, int *iStreamType, VideoDataRecord *mPool);

VA_POOL *VAPool_colloc(unsigned int nMaxFrameSize,unsigned int nBufferSize)
{
	VA_POOL *pVA = NULL;
	pVA = calloc(1, sizeof(VA_POOL));
	if(pVA == NULL)
	{
		return NULL;
	}	
	pVA->g_vaBuf = calloc(1, nBufferSize);
	if(pVA->g_vaBuf == NULL)
	{
		free(pVA);
		pVA = NULL;
		return NULL;
	}

	pVA->g_oBufMaxLen = nBufferSize;
	if(nMaxFrameSize > VSPOOLMAXSIZE)
	{
		pVA->g_poolSize = VSPOOLMAXSIZE;
	}
	else
	{
		pVA->g_poolSize = nMaxFrameSize;
	}
	VAPool_InitOthers(pVA);//init the default value
	    
	return pVA;
	
}

void VAPool_free(VA_POOL *pVAPool)
{
	if(pVAPool == NULL) return;

	if(pVAPool->g_vaBuf != NULL)
	{
		free(pVAPool->g_vaBuf);
		pVAPool->g_vaBuf = NULL;
	}

	free(pVAPool);
	pVAPool = NULL;
}

void VAPool_InitOthers(VA_POOL *pVAPool)
{	

    if(pVAPool == NULL) return;
    
	pVAPool->g_readIndex = 0;
	pVAPool->g_writeIndex = 0;
	pVAPool->g_bLost = 0;
	pVAPool->g_bAgain = 0;
	pVAPool->g_writePos = 0;
	pVAPool->g_readPos = 0;
	pVAPool->g_CurPos = 0;
	pVAPool->g_haveFrameCnt = 0;
	pVAPool->g_allDataSize = 0;
	pVAPool->g_allDataSize = 0;
	pVAPool->g_allDataSize = 0;	
}

/*
往pool写数据
*/
static int VAPool_Write(VA_POOL *pVAPool,char* pData,unsigned int nSize,sVAFrameInfo vaFrame)
{
    
	if(pVAPool == NULL || pData == NULL || nSize == 0 || pVAPool->g_vaBuf == NULL) return -1;
		
	unsigned int tmpMaxLen = pVAPool->g_oBufMaxLen;

	if (pVAPool->g_haveFrameCnt == (pVAPool->g_poolSize)-2  \
		|| ((pVAPool->g_allDataSize + nSize) > tmpMaxLen) \
		|| ((pVAPool->g_CurPos + nSize) > tmpMaxLen))
	{
		pVAPool->g_bLost = 1;
		return 0;
	}

	pVAPool->g_bLost = 0;

	if( pVAPool->g_writeIndex == pVAPool->g_poolSize ) 
	{
		pVAPool->g_writeIndex = 0;
		pVAPool->g_CurPos = 0;
	}
	
	memcpy( pVAPool->g_vaBuf + pVAPool->g_CurPos ,pData,nSize );

	pVAPool->g_oFramelist[pVAPool->g_writeIndex].oVAFrame.isIframe = vaFrame.isIframe;
	pVAPool->g_oFramelist[pVAPool->g_writeIndex].oVAFrame.iEnc = vaFrame.iEnc;
	pVAPool->g_oFramelist[pVAPool->g_writeIndex].oVAFrame.iStreamtype = vaFrame.iStreamtype;
	pVAPool->g_oFramelist[pVAPool->g_writeIndex].oVAFrame.u64PTS = vaFrame.u64PTS;

	pVAPool->g_oFramelist[pVAPool->g_writeIndex].nPos = pVAPool->g_CurPos;
	pVAPool->g_CurPos += nSize;
	pVAPool->g_allDataSize += nSize;
	pVAPool->g_oFramelist[pVAPool->g_writeIndex].nSize = nSize;
	pVAPool->g_writeIndex ++ ;
	pVAPool->g_haveFrameCnt++; 	

	return 0;
	
}

/*
从pool 读数据放到 vaFrame  pData
*/
static int VAPool_Read(VA_POOL *pVAPool,char** pData,unsigned int* nSize,sVAFrameInfo *vaFrame)
{
    
	if(nSize == NULL || pVAPool == NULL || pVAPool->g_vaBuf == NULL)	return 0;

	*nSize = 0;
    
	if ((pVAPool->g_allDataSize == 0) || (pVAPool->g_haveFrameCnt == 0)){
		pVAPool->g_bAgain = 0;
		pVAPool->g_writePos = 0;
		pVAPool->g_readPos = 0;
		pVAPool->g_CurPos = 0;
		pVAPool->g_allDataSize= 0;
		pVAPool->g_haveFrameCnt = 0;
		pVAPool->g_writeIndex = 0;
		pVAPool->g_readIndex = 0;	
		return 0;
	}

	//添加一个缓冲区，去掉了线程锁.加快读取效率。
	//if(pVAPool->g_haveFrameCnt < 2) 	return 0;

	//Read to the end.
	if( pVAPool->g_readIndex >= pVAPool->g_poolSize) 
	{
		pVAPool->g_readIndex = 0;
	}

	if(pVAPool->g_oFramelist[pVAPool->g_readIndex].nSize == 0)
	{	
		DEBUG_PRT( "pVAPool->g_readIndex:%d have some err!\n",pVAPool->g_readIndex);
		pVAPool->g_readIndex ++;
		pVAPool->g_haveFrameCnt --;
		return 0;
	}
	
	*pData =   pVAPool->g_vaBuf + pVAPool->g_oFramelist[pVAPool->g_readIndex].nPos;
	*nSize = pVAPool->g_oFramelist[pVAPool->g_readIndex].nSize;
    
	if(vaFrame){
		vaFrame->isIframe = pVAPool->g_oFramelist[pVAPool->g_readIndex].oVAFrame.isIframe;
		vaFrame->iEnc = pVAPool->g_oFramelist[pVAPool->g_readIndex].oVAFrame.iEnc;
		vaFrame->iStreamtype = pVAPool->g_oFramelist[pVAPool->g_readIndex].oVAFrame.iStreamtype;
		vaFrame->u64PTS = pVAPool->g_oFramelist[pVAPool->g_readIndex].oVAFrame.u64PTS;

	}
    
	//Read Data to Pool
	pVAPool->g_haveFrameCnt --;
	pVAPool->g_allDataSize -= pVAPool->g_oFramelist[pVAPool->g_readIndex].nSize ;
	pVAPool->g_readPos = pVAPool->g_oFramelist[pVAPool->g_readIndex].nPos;
	pVAPool->g_oFramelist[pVAPool->g_readIndex].nSize = 0;
	pVAPool->g_oFramelist[pVAPool->g_readIndex].nPos = 0;

	memset(&(pVAPool->g_oFramelist[pVAPool->g_readIndex].oVAFrame),0,sizeof(sVAFrameInfo));
	pVAPool->g_readIndex ++ ;

	return *nSize;

}


int StreamPoolInit(int pStreamType)
{

	if(pStreamType == ENUM_VS_REC && g_oPoolList[pStreamType] == NULL)
	{
		g_oPoolList[pStreamType] = VAPool_colloc(150,MAX_VIDEODATA_TF);//5s
	}
	else if(g_oPoolList[pStreamType] == NULL)
	{
		g_oPoolList[pStreamType] = VAPool_colloc(30,MAX_VSLIVE_POOL);//1s 
		return TRUE;
	}
	return 0;
	
}
//Reset the stream pool 
int StreamPoolReset(int pStreamType)
{
	if(g_oPoolList[pStreamType] == NULL)
	{
		VAPool_InitOthers(g_oPoolList[pStreamType]);
		return TRUE;
	}	
	return FALSE;
}


int StreamPoolRealase(int pStreamType)
{
	if(g_oPoolList[pStreamType] != NULL)
	{
		VAPool_free(g_oPoolList[pStreamType]);
		return TRUE;
	}
	return FALSE;
}


int StremPoolWrite(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame)
{
    	int nRet = FALSE;

    pthread_mutex_lock( &g_MutexLock_STREAM_POOL );
	VA_POOL *pTmp = g_oPoolList[pStreamType];
	if(pTmp == NULL)
	{
		DEBUG_PRT("g_oPoolList[%d] is NULL \n",pStreamType);
		pthread_mutex_unlock( &g_MutexLock_STREAM_POOL );
		return FALSE;
	}	
	nRet = VAPool_Write(pTmp,pData,nSize,vaFrame);
	pthread_mutex_unlock( &g_MutexLock_STREAM_POOL );

	return nRet;
}

int StremPoolRead(int pStreamType,char** pData,unsigned int* nSize,sVAFrameInfo *vaFrame)
{
    	int nRet = FALSE;

    pthread_mutex_lock( &g_MutexLock_STREAM_POOL );
	VA_POOL *pTmp = g_oPoolList[pStreamType];
	if(pTmp == NULL)
	{
		DEBUG_PRT("g_oPoolList[%d] is NULL \n",pStreamType);
		pthread_mutex_unlock( &g_MutexLock_STREAM_POOL );
		return FALSE;	
	}		
	nRet = VAPool_Read(pTmp,pData,nSize,vaFrame);
	pthread_mutex_unlock( &g_MutexLock_STREAM_POOL );

	return nRet;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif 
#endif /* End of #ifdef __cplusplus */

