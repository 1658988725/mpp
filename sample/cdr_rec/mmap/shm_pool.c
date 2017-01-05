
#include "shm_pool.h"  


static int g_videoshmid = -1;
static int g_videosem_id = -1;



static SHARED_USE_ST *g_oVedioStreammmap = NULL;
static SHARED_USE_ST *g_oAudioStreammmap = NULL;

static int MAAPWriteDataToPool(int pStreamType,char* pData,unsigned int nSize,int IFrame,int iResType, int iStreamType, SHARED_USE_ST *mPool);
static int MMAPReadDataFromPool(int pStreamType,char** pData,unsigned int* nSize,int *IFrame,int *iResType, int *iStreamType, SHARED_USE_ST *mPool);

static void init_sem(void);
static int set_semvalue(void);
static void del_semvalue(void);
static int semaphore_lock(void);
static int semaphore_unlock(void);


//Init the stream pool 
int MMAPPoolInit(int pStreamType)
{  
	void *shared_memory=NULL;	
	//struct shared_use_st *shared_stuff;  

	int nRet = 0;
	g_videoshmid=shmget((key_t)VIDEOPOOL_KEYID,sizeof(SHARED_USE_ST),0666|IPC_CREAT);	
	if(g_videoshmid==-1)  
	{  
		MMAPDEBUG_PRT("shmget failed\n");	
		return -1;
	}  

	shared_memory=shmat(g_videoshmid,(void *)0,0);	
	if(shared_memory==(void *)-1)  
	{  
		MMAPDEBUG_PRT("shmat failed\n");  
		return -1;
	}  
	MMAPDEBUG_PRT("Memory attached at %X\n",(int)shared_memory);
	g_oVedioStreammmap=(struct SHARED_USE_ST *)shared_memory;  
	
	init_sem();
	set_semvalue();

	MMAPPoolReset(ENUM_STREM_VIEDO);
	return 0;
	
}


//Reset the stream pool 
int MMAPPoolReset(int pStreamType)
{
	if(g_videoshmid != -1 && g_oVedioStreammmap != NULL )
	{
		g_oVedioStreammmap->g_readIndex		=0;  //Read index,encode thread write index.
		g_oVedioStreammmap->g_writeIndex	=0; //Write to sd index.
		g_oVedioStreammmap->g_bLost			=0;	   //Need lost buffer.The sd write too slow
		g_oVedioStreammmap->g_bAgain		=0;     //Reset
		g_oVedioStreammmap->g_writePos		=0;   //
		g_oVedioStreammmap->g_readPos		=0;
		g_oVedioStreammmap->g_allDataSize	=0;//All data leng
		g_oVedioStreammmap->g_CurPos		=0;
		g_oVedioStreammmap->g_haveFrameCnt	=0;
		MMAPDEBUG_PRT("MMAPPoolReset OK\n");
	}	
}

//Realase the stream pool 
int MMAPPoolRealase(int pStreamType)
{
	if(g_videoshmid != -1)
	{
		//Delete cfg mmap.
		shmdt((void *)g_oVedioStreammmap);
		g_oVedioStreammmap = NULL;
		shmctl(g_videoshmid, IPC_RMID, NULL) ;
		g_videoshmid = -1;		
	}	
	del_semvalue();
}

int MAAPWriteVedioData(int pStreamType,char* pData,unsigned int nSize,int IFrame,int iResType, int iStreamType)
{
	MMAPDEBUG_PRT("Write len:%d\n",nSize);
	//pthread_mutex_lock( &g_MutexLock_VEDIO_TF );
	semaphore_lock();
	MAAPWriteDataToPool(pStreamType,pData,nSize,IFrame,iResType, iStreamType, g_oVedioStreammmap);	
	semaphore_unlock();
	//pthread_mutex_unlock( &g_MutexLock_VEDIO_TF );
	MMAPDEBUG_PRT("Write len: finish%d\n",nSize);
}

static int MAAPWriteDataToPool(int pStreamType,char* pData,unsigned int nSize,int IFrame,int iResType, int iStreamType, SHARED_USE_ST *mPool)
{
	if(pData == NULL || nSize == 0 || mPool == NULL || mPool->g_Buf == NULL)
	{
		MMAPDEBUG_PRT("Parem error\n");
		return 0;
	}

	long tmpMaxLen = 0;
	if(pStreamType == ENUM_STREM_AUDIO)
	{
		tmpMaxLen = MAX_AUDIO_BUFFSIZE;
	}
	else if(pStreamType == ENUM_STREM_VIEDO)
	{
		tmpMaxLen = MAX_VIDEO_BUFFSIZE;
	}

	//Judge Data.
	if ( ((mPool->g_CurPos + nSize) > tmpMaxLen) \
		|| ((mPool->g_allDataSize + nSize) > tmpMaxLen) \
		|| (nSize > 190000) \
		|| (abs(mPool->g_writeIndex - mPool->g_readIndex) > (POOLSIZE-1)) )
	{
		MMAPDEBUG_PRT("stream type:%d,g_allDataSize:%d, nSize:%d, g_writeIndex:%d, g_readIndex:%d....\n ",pStreamType,mPool->g_allDataSize, nSize, mPool->g_writeIndex, mPool->g_readIndex);
		mPool->g_bLost = 1;
		usleep(10);
		return 0;
	}
	
#if 0
	if( mPool->g_bLost == 1 && IFrame != 1 )
	{
		MMAPDEBUG_PRT( "Push Video lost--ccccccc\n" );
		return 0;
	}
#endif

	mPool->g_bLost = 0;

	if( mPool->g_writeIndex == POOLSIZE ) 
	{
		mPool->g_writeIndex = 0;
	}

	//Add Data to Pool
   MMAPDEBUG_PRT("mPool->g_CurPos:%d\n",mPool->g_CurPos);
   memcpy( &(mPool->g_Buf[mPool->g_CurPos]),pData,nSize );
   mPool->g_item[mPool->g_writeIndex].IFrame = IFrame;
   mPool->g_item[mPool->g_writeIndex].iEnc = iResType;
   mPool->g_item[mPool->g_writeIndex].StreamType = iStreamType;

   mPool->g_item[mPool->g_writeIndex].nPos = mPool->g_CurPos;
   mPool->g_CurPos += nSize;
   mPool->g_allDataSize += nSize;
   mPool->g_item[mPool->g_writeIndex].nSize = nSize;
   mPool->g_writeIndex ++ ;
   mPool->g_haveFrameCnt++;

   return 0;

}


int MAAPReadVedioData(int pStreamType,char** pData,unsigned int* nSize,int *IFrame,int *iResType, int *iStreamType)
{
	int nRet = 0;
	semaphore_lock();
	nRet = MMAPReadDataFromPool(pStreamType,pData,nSize,IFrame,iResType, iStreamType, g_oVedioStreammmap);
	semaphore_unlock();
	return nRet;
}

static int MMAPReadDataFromPool(int pStreamType,char** pData,unsigned int* nSize,int *IFrame,int *iResType, int *iStreamType, SHARED_USE_ST *mPool)
{
	if(nSize == NULL || mPool == NULL || mPool->g_item == NULL)
	{
		MMAPDEBUG_PRT("Parem error\n");
		return 0;
	}

	*nSize = 0;

	if ((mPool->g_allDataSize == 0) || (mPool->g_haveFrameCnt == 0))
	{
		mPool->g_bAgain = 0;
		mPool->g_writePos = 0;
		mPool->g_readPos = 0;
		mPool->g_CurPos = 0;
		mPool->g_allDataSize= 0;
		mPool->g_haveFrameCnt = 0;
		mPool->g_writeIndex = 0;
		mPool->g_readIndex = 0;
		MMAPDEBUG_PRT("NO DATA in pool.\r\n");
		return 0;
	}

	//Read to the end.
	if( mPool->g_readIndex == POOLSIZE ) 
	{
		mPool->g_readIndex = 0;
		MMAPDEBUG_PRT("g_readIndex == POOLSIZE \r\n");
	}
	
	if( mPool->g_item[mPool->g_readIndex].nSize == 0  )
	{
		MMAPDEBUG_PRT( "have some err!\n" );
		return 0;
	}

	//memcpy( pData,mPool->g_videoBuf + mPool->g_VideoData[mPool->g_readIndex].nPos,mPool->g_VideoData[mPool->g_readIndex].nSize);
	*pData =   mPool->g_Buf[mPool->g_item[mPool->g_readIndex].nPos];
	*nSize = mPool->g_item[mPool->g_readIndex].nSize;
	if(IFrame)
	{
		//For audio iFrame == NULL
		*IFrame = mPool->g_item[mPool->g_readIndex].IFrame;
	}
	if(iResType)
	{
		//For audio iResType == NULL
		*iResType =mPool->g_item[mPool->g_readIndex].iEnc; 
	}
	*iStreamType = mPool->g_item[mPool->g_readIndex].StreamType;	

	//Read Data to Pool
    mPool->g_haveFrameCnt --;
    mPool->g_allDataSize -= mPool->g_item[mPool->g_readIndex].nSize ;
    mPool->g_readPos = mPool->g_item[mPool->g_readIndex].nPos;
    mPool->g_item[mPool->g_readIndex].nSize = 0;
    mPool->g_item[mPool->g_readIndex].nPos = 0;
    mPool->g_item[mPool->g_readIndex].IFrame = 0;
    mPool->g_item[mPool->g_readIndex].iEnc = 0;
    mPool->g_item[mPool->g_readIndex].StreamType = 0;
    mPool->g_readIndex ++ ;
	MMAPDEBUG_PRT("ReadDdata size %d\r\n",*nSize);
	return *nSize;
}


static void init_sem()
{	
	g_videosem_id = semget((key_t)VIDIOSEM_KEYID, 1, 0666 | IPC_CREAT);
}

static int set_semvalue(void)
{
    union semun sem_union;

    sem_union.val = 1;
    if(semctl(g_videosem_id, 0, SETVAL, sem_union) == -1) return 0;
    return 1;
}

static void del_semvalue(void)
{
    union semun sem_union;

    if(semctl(g_videosem_id, 0, IPC_RMID, sem_union) == -1)
        MMAPDEBUG_PRT("Failed to delete semaphore\n");
}

//P
static int semaphore_lock(void)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;

	while(semop(g_videosem_id, &sem_b, 1) == -1)
	{
		usleep(1);
	}

	MMAPDEBUG_PRT("semaphore_lock finish\n");
    return 1;
}

//V
static int semaphore_unlock(void)
{
    struct sembuf sem_b;

	MMAPDEBUG_PRT("semaphore_unlock start\n");

    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
	while(semop(g_videosem_id, &sem_b, 1) == -1)
	{
		usleep(1);
	}
	MMAPDEBUG_PRT("semaphore_unlock finish\n");
    return 1;
}
