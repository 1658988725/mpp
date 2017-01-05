#include <string.h>
#include <stdio.h>
#include "cdr_mpp.h"
#include "cdr_vedio.h"
#include "cdr_app_service.h"
#include "cdr_XmlLog.h"
#include "cdr_config.h"
#include "cdr_vedio.h"

#include "cdr_exif.h"
#include "hi_comm_isp.h"
#include "mpi_isp.h"
#include "cdr_comm.h"

/***********************Data flow:***************************
	Sersor--> bufferpool -->Callbac function --> net or sd.
************************************************************/

static cdr_stream_callback g_fun_callbak[CDR_CALLBACK_MAX];
VENC_GETSTREAM_CH_PARA_S gsjpeg_stPara;
VENC_GETSTREAM_CH_PARA_S gsqcif_stPara;


//static VENC_CHN	g_jepgchn = 2;
//static VENC_CHN	g_indexjepgchn = 3;

#define NORMAL_JPG 1
#define GSENSOR_JPG 2
#define NORMAL_QCIF 3
#define AVIDEO_QCIF 4
#define JPGPRE_QCIF 5

extern GPS_INFO g_GpsInfo;

//mv gcif to zip.
void cdr_zip_qcif(void)
{
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};			
	time(&curTime);
	ptm = gmtime(&curTime);

	memset(tmp,0x00,256);
	//don't need path in the zip file.
	sprintf(tmp, "zip -qj /mnt/mmc/INDEX/%04d%02d%02d%02d%02d%02d.zip /mnt/mmc/INDEX/*.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	cdr_system(tmp);
	cdr_system("rm -rf /mnt/mmc/INDEX/*jpg");
	printf("%s zip qcif ok..");
	
}
void cdr_captue_finish(int type,char *fullname,char *name)
{
	//printf("%s %d %s %s \r\n",__FUNCTION__,__LINE__,fullname,name);

	switch(type)
	{
		case NORMAL_JPG:
			cdr_add_log(ePHOTO,"media",name);
			//cdr_write_exif_to_jpg(fullname,g_GpsInfo);
		break;
		case GSENSOR_JPG:			
			cdr_add_log(eGPHOTO,"media",name);
	        //cdr_write_exif_to_jpg(fullname,g_GpsInfo);            
	        //memcpy(g_cdr_systemconfig.recInfo.strAVPicName,fullname,strlen(fullname));
		break;
#if 0			
		case NORMAL_QCIF:
		case AVIDEO_QCIF:
		break;
#endif		
		default:
		break;
	}
}

HI_S32 save_stream_to_jepg(VENC_STREAM_S *pstStream, char *pJepgName)
{
    //char acFile[FILE_NAME_LEN]  = {0};
    //char acFile_dcf[FILE_NAME_LEN]  = {0};
    FILE *pFile;
    HI_S32 s32Ret;
    
    pFile = fopen(pJepgName, "wb");
    if (pFile == NULL)
    {
        SAMPLE_PRT("open file err\n");
        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_VENC_SaveJPEG(pFile, pstStream);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("save snap picture failed!\n");
        return HI_FAILURE;
    }
    fflush(pFile);
    if(pFile!=NULL)
    {
      fclose(pFile);
      pFile = NULL;
    }
           
    return HI_SUCCESS;
}

HI_S32 cdr_save_pre_jpg(char *pJpgpreName)
{
	//printf("pJpgpreName:%s\r\n",pJpgpreName);
	char *p = NULL;	
	char chPreName[256],tmp[256];
	memcpy(tmp,pJpgpreName,256);
	p = strchr(tmp,'.');
	*p = '\0';
	sprintf(chPreName,"%s_pre.jpg",tmp);
	return cdr_save_jpg(gsqcif_stPara.s32VencChn,chPreName);	

}

HI_S32 cdr_save_jpg(VENC_CHN VencChn,char *pJpgName)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
	VENC_RECV_PIC_PARAM_S stRecvParam;
    
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = 1;
    s32Ret = HI_MPI_VENC_StartRecvPicEx(VencChn,&stRecvParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return HI_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
    s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
    	 SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return HI_FAILURE;
    }

    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);
    
    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;
    s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0) 
    {
        SAMPLE_PRT("snap select failed!\n");
        return HI_FAILURE;
    }
    else if (0 == s32Ret) 
    {
        SAMPLE_PRT("snap time out!\n");
        return HI_FAILURE;
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }
			
			/*******************************************************
			 suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
			 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
			 {
				SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
				return HI_SUCCESS;
			 }
			*******************************************************/
			if(0 == stStat.u32CurPacks)
			{
				  SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
				  return HI_SUCCESS;
			}
            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                SAMPLE_PRT("malloc memory failed!\n");
                return HI_FAILURE;
            }

            stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, -1);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }
			s32Ret = save_stream_to_jepg(&stStream,pJpgName);
            //s32Ret = SAMPLE_COMM_VENC_SaveSnap(&stStream, bSaveJpg, bSaveThm);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
            if (s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
	    }
    }
    /******************************************
     step 4:  stop recv picture
    ******************************************/
    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return HI_FAILURE;
    }


    return HI_SUCCESS;
}


int cdr_qcif_thread(void)
{
	char fullJpgName[256];//全路径名字
	char qcifName[256];	  //名字	
	while( gsqcif_stPara.bThreadStart)
	{
		sleep(20);
		memset(fullJpgName,0x00,256);
		get_jpg_tm_name("/mnt/mmc/INDEX/",fullJpgName,qcifName);
		cdr_save_jpg(gsqcif_stPara.s32VencChn,fullJpgName);		
	}	 
	return 1;
}

void get_Gjpg_tm_name(char*path,char *pfullFileName,char *pName)
{
	if (NULL == path || NULL == pfullFileName || pName == NULL)
	{
		return -1;
	}
	memset(pfullFileName, '\0', sizeof(pfullFileName));
	memset(pName, '\0', sizeof(pName));
		
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};

	
	time(&curTime);
	ptm = gmtime(&curTime);
	
	memset(tmp,0x00,256);
	sprintf(tmp, "%sG%04d%02d%02d%02d%02d%02d.jpg", path,ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pfullFileName, tmp);


	memset(tmp,0x00,256);
	sprintf(tmp, "G%04d%02d%02d%02d%02d%02d.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pName, tmp);
	
	return 0;	

}
void get_jpg_tm_name(char*path,char *pfullFileName,char *pName)
{
	if (NULL == path || NULL == pfullFileName || pName == NULL)
	{
		return -1;
	}
	memset(pfullFileName, '\0', sizeof(pfullFileName));
	memset(pName, '\0', sizeof(pName));
		
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};

	
	time(&curTime);
	ptm = gmtime(&curTime);
	
	memset(tmp,0x00,256);
	sprintf(tmp, "%s%04d%02d%02d%02d%02d%02d.jpg", path,ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pfullFileName, tmp);


	memset(tmp,0x00,256);
	sprintf(tmp, "%04d%02d%02d%02d%02d%02d.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pName, tmp);
	
	return 0;	
}

static char s_cFullJpgName[256] = {0};

int GetFullJpgName(char *pDst)
{
  memcpy(pDst,s_cFullJpgName,strlen(s_cFullJpgName));
  
  return 0;
}

int SetFullJpgName(char *pSrc)
{    
  memcpy(s_cFullJpgName,pSrc,strlen(pSrc)); 
  return 0;
}

#if 0
int cdr_jpeg_thread(void)
{
	char fullJpgName[256];//全路径名字
	char jpgName[256];	  //名字
	while( gsjpeg_stPara.bThreadStart)
	{
		if(gsjpeg_stPara.nsnapflag < 0)
			gsjpeg_stPara.nsnapflag = 0;
		
		if(gsjpeg_stPara.exsnapflag < 0)
			gsjpeg_stPara.exsnapflag = 0;
		
		usleep(1000);
		
		//Normal photo.
		if(gsjpeg_stPara.nsnapflag > 0)
		{
			//memset(chJpgName,0x00,sizeof(chJpgName));
			cdr_play_audio(CDR_AUDIO_IMAGECAPUTER,0);			
						
			get_jpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
			cdr_save_jpg(gsjpeg_stPara.s32VencChn,fullJpgName);
			sleep(1);
			gsjpeg_stPara.nsnapflag --;
			ack_capture_update(jpgName);
			printf("photo %s ok\r\n",fullJpgName);
			cdr_captue_finish(NORMAL_JPG,fullJpgName,jpgName);
            SetFullJpgName(fullJpgName);
		}

		//G pthoto...
		if(gsjpeg_stPara.exsnapflag > 0)
		{
			//memset(chJpgName,0x00,sizeof(chJpgName));
			cdr_play_audio(CDR_AUDIO_IMAGECAPUTER,0);	
			get_Gjpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
			cdr_save_jpg(gsjpeg_stPara.s32VencChn,fullJpgName);
			sleep(1);
			gsjpeg_stPara.exsnapflag --;
			ack_capture_update(jpgName);
			cdr_captue_finish(GSENSOR_JPG,fullJpgName,jpgName);
			printf("photo %s ok\r\n",fullJpgName);
		}
				
	}    
	return 1;
}
#endif


#if 0
//pic
void init_jpeg_thread()
{
	pthread_t tfid;
	int ret = 0;

	gsjpeg_stPara.bThreadStart = HI_TRUE;
	gsjpeg_stPara.s32VencChn = 2;
	ret = pthread_create(&tfid, NULL, (void *)cdr_jpeg_thread, NULL);
	if (ret != 0)
	{
		printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
		return -1;
	}
	pthread_detach(tfid);
	return 0;
}
#endif

void cdr_capture_jpg(int type)
{
	char fullJpgName[256];//全路径名字
	char jpgName[256];	  //名字
	gsjpeg_stPara.s32VencChn = 2;

	int jpgtype = NORMAL_JPG;

	cdr_play_audio(CDR_AUDIO_IMAGECAPUTER,0);	
	if(type == 0)
	{		
		get_jpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
		jpgtype = NORMAL_JPG;
	}
	else
	{
		get_Gjpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
		jpgtype = GSENSOR_JPG;
	}
	if (0 != access(fullJpgName, W_OK))
	{
		cdr_save_pre_jpg(fullJpgName);
		cdr_save_jpg(gsjpeg_stPara.s32VencChn,fullJpgName);			
	}
	
	ack_capture_update(jpgName);
	cdr_captue_finish(jpgtype,fullJpgName,jpgName);	

}

void cdr_capture_qcif(int type)
{
	if(type == 0)
		gsqcif_stPara.nsnapflag ++;	
	else
		gsqcif_stPara.exsnapflag ++;	
}


//每20秒生成一个index图片。
void init_indexqcif_thread()
{
	pthread_t tfid;
	int ret = 0;

	gsqcif_stPara.bThreadStart = HI_TRUE;
	gsqcif_stPara.s32VencChn = 3;

	ret = pthread_create(&tfid, NULL, (void *)cdr_qcif_thread, NULL);
	if (ret != 0)
	{
		printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
		return -1;
	}
	pthread_detach(tfid);
	return 0;

}


//Read buff from pool and call the callback fuction.
void cdr_rec_read_pool_thread(void)
{
	char *pData = NULL;
	int nSize = 0;
	sVAFrameInfo vaFrame;
    
	/*write video audio frame*/
	cdr_stream_callback _fcallbak = g_fun_callbak[CDR_REC_TYPE];
	while (1)
	{
		_fcallbak = g_fun_callbak[CDR_REC_TYPE];
		usleep(20);
		if(_fcallbak == NULL)	continue;
        
		//Get video.
		pData = NULL;
		nSize = 0;
		memset(&vaFrame,0,sizeof(sVAFrameInfo));
		StremPoolRead(ENUM_VS_REC, &pData, &nSize, &vaFrame);	
		if(pData && nSize > 0)
		{
			_fcallbak(STREAM_VIDEO,pData,nSize,vaFrame);
		}

		//Get audio.
		pData = NULL;
		nSize = 0;
		memset(&vaFrame,0,sizeof(sVAFrameInfo));

		StremPoolRead(ENUM_AS_REC, &pData, &nSize, &vaFrame);	
		if(pData && nSize > 0)
		{
			_fcallbak(STREAM_AUDIO,pData,nSize,vaFrame);
		}
		
	}
	return;
}



//Read buff from pool and call the callback fuction.
//先读后写
void cdr_live_read_pool_thread(void)
{
	char *pData = NULL;
	int nSize = 0;
	sVAFrameInfo vaFrame;
	/*write video audio frame*/
	cdr_stream_callback _fcallbak = g_fun_callbak[CDR_LIVE_TYPE];
	while (1)
	{
		_fcallbak = g_fun_callbak[CDR_LIVE_TYPE];
		usleep(20);
		        
		//Get video.
		pData = NULL;
		nSize = 0;
		
		memset(&vaFrame,0,sizeof(sVAFrameInfo));
		StremPoolRead(ENUM_VS_LIVE, &pData, &nSize, &vaFrame);//读 pool 中的数据到 pData vaFrame中
		if(pData && nSize > 0 && _fcallbak != NULL)
		{
            //int live_stream_callbak(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame)
			_fcallbak(STREAM_VIDEO,pData,nSize,vaFrame);//写write video rtsp发送 消费
		}

		//Get audio.
		pData = NULL;
		nSize = 0;
		memset(&vaFrame,0,sizeof(sVAFrameInfo));
		StremPoolRead(ENUM_AS_LIVE, &pData, &nSize, &vaFrame);	
		if(pData && nSize > 0)
		{
			_fcallbak(STREAM_AUDIO,pData,nSize,vaFrame);//写音频数据
		}		
	}
	return;
}



//create two thread.
//one for read stream from sensor to buffer pool.
//one for bufferpool to callback.
int init_rec_thread(void)
{
    int res = 0;
     res = start_get_rec_video_stream();	//获取rec stream 线程
	return res; 	
}



//直播线程
//create two thread.
//one for read stream from sensor to buffer pool.
//one for bufferpool to callback.
int init_live_thread(void)
{
	pthread_t tfid = 0;
    int ret = 0;
	
	//Create Read thread.one for read stream from sensor to buffer pool.
	//将pool中的数据 rtsp发送 消息出去
	ret = pthread_create(&tfid, NULL, (void *)cdr_live_read_pool_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);

	//获取live video stream 线程 	//从源头sensor读视频数据 存入到pool中
	ret = start_get_live_video_stream();

    return ret;
}



/*
function:cdr_mpp_init(void)
1.create bufferpool.
2.init hisi mpp.(sersor isp ..)
3.init region thread.
4.create read buffer thread.and write to bufferpool.
5.create read buffer from bufferpool thread.
return:0 OK;-1 failed.
*/
int cdr_mpp_init(void)
{
	int nRet = 0,i;
	for(i = 0;i<CDR_CALLBACK_MAX;i++)
	{
		g_fun_callbak[i] = NULL;
	}

	StreamPoolInit(ENUM_VS_LIVE);
	StreamPoolInit(ENUM_AS_LIVE);	
	StreamPoolInit(ENUM_VS_REC);
	StreamPoolInit(ENUM_AS_REC);	
		
	cdr_videoInit();//video and osd
	cdr_audioInit();	

	cdr_isp_init();	

	//初始化录相线程
	init_rec_thread();
	//初始化直播线程
	init_live_thread();	

	//init_jpeg_thread();
	init_indexqcif_thread();

	if(Get_BootVoiceCtrl() == 0x01)cdr_play_audio(CDR_AUDIO_BOOTVOICE,0);
	
	return nRet;
}

/*
function:cdr_mpp_release(void)
1.stop thread (read & send thread.)
2.release bufferpool.
2.uninit hisi mpp.
return:0 OK;-1 failed.
*/
int cdr_mpp_release(void)
{
	int nRet = 0;
	
	//cdr_audio_release();
	cdr_video_release();	
	
	StreamPoolRealase(ENUM_VS_REC);
	StreamPoolRealase(ENUM_AS_REC);
	StreamPoolRealase(ENUM_VS_LIVE);
	StreamPoolRealase(ENUM_AS_LIVE);
	
	return nRet;
}



/*
params:
	pStreamType	:0 video,1 audio.
	pData		:buffer
	nSize		:length of buffer
	freamType		:fame freamType.
return:

单独线程调用，等待执行完毕才会继续执行.
	阻塞函数.
	
*/
void cdr_stream_setevent_callbak(int ntype,cdr_stream_callback callback)
{	
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n");
		return;
	}
	g_fun_callbak[ntype] = callback;	
	
	int i = 0;
	for(i = 0;i<CDR_CALLBACK_MAX;i++)
	{
		if(g_fun_callbak[i] != NULL){
            printf("%s g_fun_callbak[%d] is register.\n",__FUNCTION__,i);
		}
	}
}
