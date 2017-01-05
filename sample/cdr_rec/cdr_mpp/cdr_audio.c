/******************************************************************************
  A simple program of Hisilicon HI3518 audio input/output/encoder/decoder implementation.
  Copyright (C), 2010-2012, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2012-7-3 Created
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "sample_comm.h"
#include "acodec.h"
//#include "aacenc.h"
//#include "aacdec.h"
#include "cdr_audio.h"


#include "stream_pool.h"
#include "cdr_writemov.h"

#include "cdr_app_service.h"
#include "cdr_config.h"



#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define SAMPLE_AUDIO_PTNUMPERFRM   1024//320

static PAYLOAD_TYPE_E gs_enPayloadType = PT_AAC;

static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAiAnr = HI_FALSE;
static HI_BOOL gs_bAioReSample = HI_FALSE;
static HI_BOOL gs_bUserGetMode = HI_FALSE;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAiReSmpAttr = NULL;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAoReSmpAttr = NULL;

static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: open*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 1;  


#define CDR_AUDIO_FILE 0

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;


typedef struct sAudioFileList
{
	struct sAudioFileList *pNext;
	char audioFile[256];
}AudioFileList;

typedef struct tagADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
	int nAudioPlayFlag;
    AudioFileList *stPlayList;
    pthread_t stAdPid;
} ADEC_S;

static ADEC_S gs_stAdec;

static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];


#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

#define SAVE_AACFILE 0
#if 1
/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  
    {
        return "g711a";
    }
    else if (PT_G711U == enType)  
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)  
    {
        return "adpcm";
    }
    else if (PT_G726 == enType) 
    {
        return "g726";
    }
    else if (PT_LPCM == enType)  
    {
        return "pcm";
    }
	else if(PT_AAC == enType)
	{
		return "aac";
	}
    else 
    {
        return "data";
    }
}

/******************************************************************************
* function : Open Aenc File
******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN ,"audio_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Ai -> Aenc -> file
*                       -> Adec -> Ao
******************************************************************************/
HI_S32 cdr_AUDIO_AiAencAo(HI_VOID)
{
    HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
	HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    //HI_BOOL     bSendAdec = HI_FALSE;
    FILE        *pfd = NULL;
    AIO_ATTR_S stAioAttr;
	AI_VQE_CONFIG_S stAiVqeAttr;	
	HI_VOID     *pAiVqeAttr = NULL;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;

    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;


    HI_MPI_AENC_AacInit();
    HI_MPI_ADEC_AacInit();
	
	if (1 == u32AiVqeType)
    {
	    stAiVqeAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
    	stAiVqeAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
    	stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_NOISY;
    	stAiVqeAttr.bAecOpen             = HI_TRUE;
    	stAiVqeAttr.stAecCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.stAecCfg.s8CngMode   = 0;
    	stAiVqeAttr.bAgcOpen             = HI_TRUE;
    	stAiVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bAnrOpen             = HI_TRUE;
    	stAiVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bHpfOpen             = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
    	stAiVqeAttr.bRnrOpen             = HI_FALSE;
    	stAiVqeAttr.bEqOpen              = HI_FALSE;
    	stAiVqeAttr.bHdrOpen             = HI_FALSE;
		
		pAiVqeAttr = (HI_VOID *)&stAiVqeAttr;
    }	
	else
	{
		pAiVqeAttr = HI_NULL;
	}

		
    /********************************************
      step 1: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, pAiVqeAttr,u32AiVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = 1;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return s32Ret;
            }
        }
        printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }

#if 1
    /********************************************
      step 5: start Adec & Ao. ( if you want )
    ********************************************/
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, PT_LPCM);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, AUDIO_SAMPLE_RATE_BUTT, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

	s32Ret = cdr_audio_play_thread_init();
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}	
#endif
 

#if CDR_AUDIO_FILE
	pfd = SAMPLE_AUDIO_OpenAencFile(AdChn, gs_enPayloadType);
	if (!pfd)
	{
		SAMPLE_DBG(HI_FAILURE);
		return HI_FAILURE;
	}		 
#endif

	//32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AeChn, AdChn, pfd);
	s32Ret = cdr_audio_create_thread(AeChn,pfd);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}


    return HI_SUCCESS;
}
HI_VOID SAMPLE_AUDIO_Usage(HI_VOID)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  start AI to AO loop\n");
    printf("\t1:  send audio frame to AENC channel from AI, save them\n");
    printf("\t2:  read audio stream from file, decode and send AO\n");
    printf("\t3:  start AI(AEC/ANR/ALC process), then send to AO\n");
    printf("\tq:  quit whole audio sample\n\n");
    printf("sample command:");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {

        SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

#endif



char g_chAudioBuffer[1024];

/******************************************************************************
* function : get stream from Aenc, send it  to Adec & save it to file
******************************************************************************/
void *_COMM_AUDIO_AencProc(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;
    
    FD_ZERO(&read_fds);    
    AencFd = HI_MPI_AENC_GetFd(pstAencCtl->AeChn);
    FD_SET(AencFd, &read_fds);
    sVAFrameInfo vaFrame;
    while (pstAencCtl->bStart)
    {     
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AencFd,&read_fds);
        
        s32Ret = select(AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
            break;
        }
        else if (0 == s32Ret) 
        {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }
        
        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = HI_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
#if CDR_AUDIO_FILE
            /* save audio stream to file */
            fwrite(stStream.pStream,1,stStream.u32Len, pstAencCtl->pfd);
            fflush(pstAencCtl->pfd);
#endif			

			vaFrame.iEnc = AAC;
			vaFrame.isIframe = 0;
			vaFrame.u64PTS = stStream.u64TimeStamp;
			
			memcpy(g_chAudioBuffer,stStream.pStream,stStream.u32Len);

			//printf("[%s] write len:%d\r\n",__FUNCTION__,stStream.u32Len);
			
			StremPoolWrite( ENUM_AS_REC, g_chAudioBuffer, stStream.u32Len, vaFrame);		
			//StremPoolWrite( ENUM_AS_LIVE, g_chAudioBuffer, stStream.u32Len, vaFrame);			
			
            /* finally you must release the stream */
            s32Ret = HI_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
        }    
    }
#if CDR_AUDIO_FILE
    fclose(pstAencCtl->pfd);
    pstAencCtl->bStart = HI_FALSE;
#endif		
    return NULL;
}


/******************************************************************************
* function : Create the thread to get stream from aenc and send to adec
******************************************************************************/
HI_S32 cdr_audio_create_thread(AENC_CHN AeChn,FILE *pAecFd)
{
    SAMPLE_AENC_S *pstAenc = NULL;
	
#if CDR_AUDIO_FILE

    if (NULL == pAecFd)
    {
        return HI_FAILURE;
    }
#endif

    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
    //pstAenc->AdChn = AdChn;
    pstAenc->bSendAdChn = HI_FALSE;
    pstAenc->pfd = pAecFd;    
    pstAenc->bStart = HI_TRUE;    
    pthread_create(&pstAenc->stAencPid, 0, _COMM_AUDIO_AencProc, pstAenc);

    return HI_SUCCESS;
}

/******************************************************************************
* function : main
******************************************************************************/
HI_S32 cdr_audioInit(void)
{
    int ret = 0;
    signal(SIGINT, SAMPLE_AUDIO_HandleSig);
    signal(SIGTERM, SAMPLE_AUDIO_HandleSig);
	gs_enPayloadType = PT_AAC;

    ret = cdr_AUDIO_AiAencAo();


    Set_VolumeCtrl(g_cdr_systemconfig.volume);
    Set_VolumeRecordingSensitivity(g_cdr_systemconfig.volumeRecordingSensitivity);
    

	return ret;   
}


HI_S32 cdr_audio_release(void)
{
	//SAMPLE_COMM_SYS_Exit();
#if 0
	int i;
    
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
#endif
	return HI_SUCCESS;

}


/******************************************************************************
* function : file -> Adec -> Ao
******************************************************************************/
HI_S32 _audio_play_file_pro()
{
	//printf("%s %d\n",__FUNCTION__,__LINE__);
	HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn = gs_stAdec.AdChn;
    HI_U8 *pu8AudioStream = NULL;
    AudioFileList *pList = NULL;
	FILE *pfd = NULL;
	//printf("%s %d\n",__FUNCTION__,__LINE__);
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        printf("%s: malloc failed!\n", __FUNCTION__);
        return -1;
    }
	//printf("%s %d\n",__FUNCTION__,__LINE__);	
	while (HI_TRUE == gs_stAdec.bStart)
	{
		//printf("%s %d\n",__FUNCTION__,__LINE__);
		pList = gs_stAdec.stPlayList;
		if(pList == NULL)
		{
			usleep(300000);
			continue;
		}
		//GPIO6_1 功放PA引脚
		HI_MPI_SYS_SetReg(0x200F00C4,0x00);
		cdr_set_gpio_mode(0x201A0400,1,1);
		HI_MPI_SYS_SetReg(0x201A0008,0xFF);

		pfd = fopen(pList->audioFile,"rb");
		if(pfd == NULL)
		{
			//打开音频文件失败.需要更新为下一个音频文件.
			pList = gs_stAdec.stPlayList->pNext;
			free(gs_stAdec.stPlayList);
			gs_stAdec.stPlayList = pList;
			continue;
		}
		while(1)
		{
			/* read from file */
			stAudioStream.pStream = pu8AudioStream;
			u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
			if (u32ReadLen <= 0)
			{

				pList = gs_stAdec.stPlayList->pNext;
				free(gs_stAdec.stPlayList);
				gs_stAdec.stPlayList = pList;
				fclose(pfd);
				pfd = NULL;
				
				//列表里面没有音频文件了.
				if(gs_stAdec.stPlayList == NULL)
				{
			        s32Ret = HI_MPI_ADEC_SendEndOfStream(s32AdecChn, HI_FALSE);
			        if (HI_SUCCESS != s32Ret)
			        {
			            printf("%s: HI_MPI_ADEC_SendEndOfStream failed!\n", __FUNCTION__);
			        }
					sleep(3);	
					//或者关闭功放.
					HI_MPI_SYS_SetReg(0x201A0008,0x00);
					//Add:20161118 蓝牙模块关闭后.IO口是高电平.喇叭会一直不停的杂音.
					//所以修改为如果蓝牙开启状态才设定为输入模式.
					if(g_cdr_systemconfig.bluetooth == 0x01)
						cdr_set_gpio_mode(0x201A0400,1,0);//设置为输入模式.否则会没有声音
				}					
				//继续播放下一个音频文件				
				 break;
			}

			/* here only demo adec streaming sending mode, but pack sending mode is commended */
			stAudioStream.u32Len = u32ReadLen;
			s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
			if(HI_SUCCESS != s32Ret)
			{
			    printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n",\
			           __FUNCTION__, s32AdecChn, s32Ret);
			    break;
			}
		}
	}
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);

    return HI_SUCCESS;
}

int cdr_audio_play_file(char *cAudioFile)
{	
	//printf("%s %s\n",__FUNCTION__,cAudioFile);
	
	AudioFileList *p,*pList = NULL;
	p = (AudioFileList*)malloc(sizeof(AudioFileList));
	if(p)
	{
		p->pNext = NULL;
		memset(p->audioFile,0x00,256);
		memcpy(p->audioFile,cAudioFile,strlen(cAudioFile));


		if(gs_stAdec.stPlayList == NULL)
		{
			HI_MPI_ADEC_ClearChnBuf(gs_stAdec.AdChn);
			gs_stAdec.stPlayList = p;
		}
		else
		{
			pList = gs_stAdec.stPlayList;
			while(pList->pNext)
				pList = pList->pNext;
			pList->pNext = p;	
		}
		return 0;
	}
	return -1;	
}


//eValue 是播放的附加文件.比如蓝牙开启关闭.FM频率数据等.
int cdr_play_audio(int type,int eValue)
{
	if(type == CDR_AUDIO_BOOTVOICE)	{
		cdr_audio_play_file("/home/audiofile/poweron.pcm");
	}
	else if(type == CDR_AUDIO_IMAGECAPUTER){
		cdr_audio_play_file("/home/audiofile/imagecapture.pcm");
	}
	else if(type == CDR_AUDIO_NOSD){
		cdr_audio_play_file("/home/audiofile/nosd.pcm");
	}
	else if(type == CDR_AUDIO_UPDATE){
		cdr_audio_play_file("/home/audiofile/systemupdate.pcm");
	}
	else if(type == CDR_AUDIO_AssociatedVideo){
		cdr_audio_play_file("/home/audiofile/photoassociatedvideo.pcm");
		if(eValue == 0x00)
			cdr_audio_play_file("/home/audiofile/close.pcm");
		else
			cdr_audio_play_file("/home/audiofile/open.pcm");
	}
	else if(type == CDR_AUDIO_FM && eValue > 0){
		
		cdr_audio_play_file("/home/audiofile/adjusttheradioto.pcm");

		int bai = eValue/1000;
		int shi = (eValue%1000)/100;
		int ge = (eValue%100)/10;
		char chTmp[256];
		//是否需要小数点后面的.
		int nNeedDot = eValue%10;
		if(bai>0)
		{
			cdr_audio_play_file("/home/audiofile/1.pcm");			
			cdr_audio_play_file("/home/audiofile/bai.pcm");	

			//105mHz
			if(ge>0)
				cdr_audio_play_file("/home/audiofile/0.pcm");	
		}
		
		if(shi >0)
		{
			sprintf(chTmp,"/home/audiofile/%d.pcm",shi);
			cdr_audio_play_file(chTmp); 
			cdr_audio_play_file("/home/audiofile/shi.pcm");		
		}

		if(ge>0)
		{
			//90 和100mHz都不需要播
			sprintf(chTmp,"/home/audiofile/%d.pcm",ge);
			cdr_audio_play_file(chTmp); 		
		}

		if(nNeedDot)
		{
			cdr_audio_play_file("/home/audiofile/dot.pcm"); 
			sprintf(chTmp,"/home/audiofile/%d.pcm",nNeedDot);
			cdr_audio_play_file(chTmp); 	
		}
		cdr_audio_play_file("/home/audiofile/mhz.pcm"); 	
		
	}
	else if(type == CDR_AUDIO_RESETSYSTEM){
		cdr_audio_play_file("/home/audiofile/reset.pcm");
	}
	else if(type == CDR_AUDIO_FORMATSD){
		cdr_audio_play_file("/home/audiofile/formatsd.pcm");
	}
	else if(type == CDR_AUDIO_SETTIMEOK){
		cdr_audio_play_file("/home/audiofile/settimeok.pcm");
	}
	else if(type == CDR_AUDIO_SYSTEMUPDATE){
		cdr_audio_play_file("/home/audiofile/systemupdate.pcm");
	}
	else if(type == CDR_AUDIO_LT8900PAIR){				
		if(eValue == 0x02)cdr_audio_play_file("/home/audiofile/startpairing.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_STARTREC){				
		cdr_audio_play_file("/home/audiofile/startrecord.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_USBOUT){				
		cdr_audio_play_file("/home/audiofile/usbout.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_DIDI){				
		cdr_audio_play_file("/home/audiofile/didi.pcm");//遥控强制录制视频hold状态
	}
}

int cdr_audio_play_thread_init(void)
{	
	gs_stAdec.stAdPid = 0;
	gs_stAdec.stPlayList = NULL;
	gs_stAdec.bStart = HI_TRUE;
	int status = pthread_create(&gs_stAdec.stAdPid, NULL, _audio_play_file_pro,NULL);
	if(status != 0)
	{
		printf("cdr_audio_play_thread_init pthread_create error\n");
	}
	pthread_detach(gs_stAdec.stAdPid);		
	return 0;	
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
