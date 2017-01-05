#include <sys/vfs.h>
#include <dirent.h>
#include "cdr_writemov.h"
#include "stream_pool.h"
#include "mp4v2/mp4v2.h"
#include "sample_comm.h"
#include "cdr_config.h"
#include "cdr_mp4_api.h"
#include "cdr_comm.h"
#include "cdr_audio.h"
#include "cdr_led.h"
#include "cdr_app_service.h"


short g_TF_flag = 0;
static short g_TF_start = 0;
int g_mp4handle = -1;


#define SD_MOUNT_PATH "/mnt/mmc/"
SD_PARAM g_SdParam;
int g_sdIsOnline = 0;
static char videobuff[1000000] = {0}; //1m


static int GetMP4Name(unsigned char *pFileName);
static unsigned long Getms_time(void);
static void cdr_load_sd(void);
int sd_update_file(void);
void cdr_video_index_file_synchronous(void);


unsigned int g_mp4filelen = 20*1000;
char chMp4StreamData[1920*1080];

int cdr_init_record(void)
{
	cdr_load_sd();
	cdr_video_index_file_synchronous();
	cdr_create_record_thread();
	return 0;
}

//Get sd status:1 mount OK.0 no sd..
int cdr_get_sd_status()
{
	return g_sdIsOnline;
}


//录相 处理线程，完成功能:将pool中的视频流 放入 mp4中
//0 sd 卡的前期处理
////1 create mp4 file
////2 往mp4文件中放数据
int cdr_record_thread_pro(void)
{
	char fileName[256];
	pthread_detach(pthread_self());
	while(1)
	{
		usleep(1);
		if(g_sdIsOnline == 0)
		{
			printf("no sd....\n");
			cdr_load_sd();
			sleep(1);
			cdr_led_contr(LED_RED,LED_STATUS_FLICK);
			continue;
		}

		if(1 != sd_update_file())
		{
			continue;
		}

		//When power off. save mp4 to sd.
		if(0 == cdr_get_powerflag())
			break;
		
		cdr_led_contr(LED_RED,LED_STATUS_DFLICK);	
		memset(fileName,0,sizeof(fileName));		
		GetMP4Name(fileName);			
		CreateMp4File_ex(fileName,g_mp4filelen);		
		
	}
	
	return 1;

}

//create record thread
int cdr_create_record_thread(void)
{
	if (0 == g_TF_flag)
	{
		int ret = -1;
		pthread_t tfid;
		g_TF_flag = 1;
		ret = pthread_create(&tfid, NULL, (void *)cdr_record_thread_pro, NULL);
		if (0 != ret)
		{
			DEBUG_PRT("create TF record thread failed!\n");
			return -1;
		}
	}
	return 1;
}

static int GetMP4Name(unsigned char *pFileName)
{
	if (NULL == pFileName)
	{
		return -1;
	}
	memset(pFileName, '\0', sizeof(pFileName));
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};
	time(&curTime);
	ptm = gmtime(&curTime);
	sprintf(tmp, "/mnt/mmc/VIDEO/%04d%02d%02d%02d%02d%02d.mp4", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pFileName, tmp);
	return 0;	
}

int Get_Current_Time(unsigned char *pTime)
{
	if (NULL == pTime)
	{
		//printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}
	memset(pTime, '\0', sizeof(pTime));
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[32] = {0};
	time(&curTime);
	ptm = gmtime(&curTime);
	sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	tmp[strlen(tmp)] = '\0';
	//printf("[%s %d] tmp: %s\n", __func__, __LINE__, tmp);   
	strcpy(pTime, tmp);
	//printf("[%s %d] pTime: %s \n", __func__, __LINE__, pTime);   

	return 0;
}

static unsigned long Getms_time(void)
{
    struct timeval tv;
    unsigned long ms = 0;    
    gettimeofday(&tv, NULL);
    ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    return ms;
}


//check MMC module.
static int CheckSDStatus()
{
    struct stat st;
    if (0 == stat("/dev/mmcblk0", &st))
    {
        if (0 == stat("/dev/mmcblk0p1", &st))
        {
            printf("...load TF card success...\n");
            return 1;
        }
        else
        {
            printf("...load TF card failed...\n");
            cdr_play_audio(CDR_AUDIO_NOSD,0);
            return 2;
        }
    }
    return 0;
}

static void cdr_load_sd(void)
{
    g_sdIsOnline = CheckSDStatus();
    if (g_sdIsOnline == 1) //index sd card inserted.
    {
        mkdir(SD_MOUNT_PATH, 0755);
        //system("umount /mnt/mmc/");
        cdr_system("mount -t vfat -o rw /dev/mmcblk0p1 /mnt/mmc/"); //mount SD.
        usleep(1000000);
        GetStorageInfo();

		mkdir("/mnt/mmc/VIDEO", 0755);
        mkdir("/mnt/mmc/PHOTO", 0755);
        mkdir("/mnt/mmc/INDEX", 0755);
		mkdir("/mnt/mmc/GVIDEO", 0755);
		cdr_system("rm -rf /mnt/mmc/VIDEO/*mp4");//删除有问题的mp4文件.		
    }
	else
	{
		g_SdParam.leftSize  = 0; 
	}
	
}


void cdr_unload_sd(void)
{
	cdr_system("sync");
	cdr_system("umount /mnt/mmc/"); //unmount SD.
}


/*******************************************
 * func: calculate SD card storage size.
 ******************************************/
int GetStorageInfo(void)
{
    struct statfs statFS;

    if (statfs(SD_MOUNT_PATH, &statFS) == -1)
    {  
        printf("error, statfs failed !\n");
        return -1;
    }

    g_SdParam.allSize   = ((statFS.f_blocks/1024)*(statFS.f_bsize/1024));
    g_SdParam.leftSize  = (statFS.f_bfree/1024)*(statFS.f_bsize/1024); 
    g_SdParam.haveUse   = g_SdParam.allSize - g_SdParam.leftSize;
    printf("scc SD totalsize=%ld...freesize=%ld...usedsize=%ld......\n", g_SdParam.allSize, g_SdParam.leftSize, g_SdParam.haveUse);
    return 0;
}

//Return :0 No enough space.1:have enough space.
int sd_update_file()
{
	int nRet = 0;
	if(g_sdIsOnline == 1 && GetStorageInfo() == 0)
	{
		//lef 1G.
		if(g_SdParam.leftSize < 1024)
		{				
			cdr_system("ls -1 /mnt/mmc/VIDEO/*4 | head -1 | xargs rm -fr");//安时间排序，删除前面的三个文件
			cdr_system("ls -1 /mnt/mmc/VIDEO/*4 | head -1 | xargs rm -fr");	
			cdr_system("ls -1 /mnt/mmc/VIDEO/*4 | head -1 | xargs rm -fr"); 
			cdr_system("sync");
			cdr_video_index_file_synchronous();
			//nRet = 0;
		}
	}
	
	GetStorageInfo();
	
	if(g_SdParam.leftSize >= 1024)
	{
		nRet = 1;
	}
	return nRet ;
}

int update_sps_pps(void)
{
	HI_S32 i;
	int iLen = 0; 

	HI_S32 s32Ret = 0;
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	VENC_CHN_STAT_S stStat;
	
	s_vencFd  = HI_MPI_VENC_GetFd(s_vencChn);
	s_maxFd   = s_vencFd + 1; //for select.
	s_vencChn = 0; //current video encode channel.	
	
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	
	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return -1;
	}

	unsigned int select_fail_count = 0;	

	while(recspslen <=0 || recppslen <=0 )
	{		
	
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );
		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
	        SAMPLE_PRT("select failed!\n");
	        usleep(1000);
	        select_fail_count ++; //modyfied: 2015-04-17.
	        if (select_fail_count > 20)
	        {
	            printf("[%s, %d] ..........select failed count: %d ...........\n", __func__, __LINE__, select_fail_count);
	            select_fail_count = 0;
	        }
	        else
	            continue;
	    }
		else if (s32Ret > 0)
		{
	        select_fail_count = 0;
			if (FD_ISSET( s_vencFd, &read_fds ))
			{
				iLen = 0;
				s32Ret = HI_MPI_VENC_Query( s_vencChn, &stStat );
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
	                usleep(1000);
	                continue;
	            }

				stStream.pstPack = pstPack;
				stStream.u32PackCount = stStat.u32CurPacks;
				s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stStream, HI_TRUE );
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_GetStream main.. failed with %#x!\n", s32Ret);
	                usleep(1000);
	                continue;
				}		

				for (i = 0; i < stStream.u32PackCount; i++)
				{
					iLen = 0;
					memcpy( videobuff + iLen, stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset);	
					iLen += stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset;

					if(stStream.pstPack[i].DataType.enH264EType == H264E_NALU_SPS)
					{
						recspslen = iLen-4;
						memcpy(recspsdata,videobuff+4,recspslen);			
					}
					else if(stStream.pstPack[i].DataType.enH264EType == H264E_NALU_PPS)
					{
						recppslen = iLen-4;
						memcpy(recppsdata,videobuff+4,recppslen);		
					}									
				}					
				
				s32Ret = HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
				if (HI_SUCCESS != s32Ret)
				{
	                SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] main.. failed with %#x!\n", s_vencChn, s32Ret);
					stStream.pstPack = NULL;
	                usleep(1000);
	                continue;
	                //break;
				}
			}	
		}	
	}

	if(pstPack) free(pstPack);
			
	printf("recspslen %d\n",recspslen);
	printf("recppslen %d\n",recppslen);
	return 0;
}


int record_mp4_finish(void* info)
{
	Mp4Context *p = info;
	char *pStr = NULL;
	printf("Record:%s finish\n",p->sName);
	if(p->oMp4ExtraInfo.sPrePicFlag)
	{
		char chPreName[256];
		sprintf(chPreName,"%s_%03d.jpg",p->oMp4ExtraInfo.sPreName,p->oMp4ExtraInfo.nCutLen+CDR_CUTMP4_EX_TIME);
		rename(p->oMp4ExtraInfo.sPreName,chPreName);

		//notify to app.
		pStr = strrchr(chPreName,'/');
		pStr++;
		ack_capture_update(pStr);	

		//G-sensor mp4.
		if(p->oMp4ExtraInfo.nOutputMp4flag)
		{
			//这个需要保证mp4demux 线程已经解析完成了这个文件.
			sleep(3);
			int nRet = 0;
			pStr = strrchr(p->oMp4ExtraInfo.sPreName,'/');
			pStr++;
			while(1)
			{
				nRet = cdr_read_mp4_ex(pStr,p->oMp4ExtraInfo.nCutLen+CDR_CUTMP4_EX_TIME,MP4GCUTOUTPATH,cdr_mp4cut_finish);				
				if(nRet == 0) break;
				sleep(1);
			}
		}	

	}
	return 0;
}

int InitAccEncoder(Mp4Context *p)
{
	if(!p) return -1;

	p->oAudioCfg.nSampleRate = 8000;
	p->oAudioCfg.nChannels = 2;
	p->oAudioCfg.nPCMBitSize = 16;
	p->oAudioCfg.nInputSamples = 1024;
	p->oAudioCfg.nMaxOutputBytes = 0;

	return 0;
}



//create mp4 file 仅写到内存，
//若写完了仅设置关闭标志，没有实际关
int CreateMp4File_ex(char *pFileName,int timelen)
{
	//int nIFrame;
	update_sps_pps();
	MP4_FRAME *pDataFrame = malloc(sizeof(MP4_FRAME));
	Mp4Context *pMp4Context = malloc(sizeof(Mp4Context));
	memset(pMp4Context,0x00,sizeof(Mp4Context));
	strcpy(pMp4Context->sName,pFileName);
	pMp4Context->nlen = timelen/1000;
	pMp4Context->nNeedAudio= 1;
	InitAccEncoder(pMp4Context);	
	pMp4Context->oAudioCfg.nSampleRate = 8000;
	
	pMp4Context->oVideoCfg.timeScale = 90000;
	pMp4Context->oVideoCfg.fps = 30;
	pMp4Context->oVideoCfg.width = 1920;
	pMp4Context->oVideoCfg.height = 1080;
	pMp4Context->spslen = recspslen;
	memcpy(pMp4Context->sps,recspsdata,recspslen);
	pMp4Context->ppslen = recppslen;
	memcpy(pMp4Context->pps,recppsdata,recppslen);
	pMp4Context->outcb = record_mp4_finish;
	g_mp4handle = cdr_mp4_create(pMp4Context);	
	free(pMp4Context);
	pMp4Context = NULL;


	HI_S32 s32Ret = 0,i;
	
	int iLen = 0;  //stream data size.
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	
	static int s_aencChn = 0;  //Venc Channel.
	static int s_aencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	
	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	AUDIO_STREAM_S stAStream;
	VENC_CHN_STAT_S stStat;
	
	s_vencFd  = HI_MPI_VENC_GetFd(s_vencChn);
	s_aencFd  = HI_MPI_AENC_GetFd(s_aencChn);
	
	s_maxFd   = (s_aencFd >s_vencFd)? s_aencFd :s_vencFd ; //for select.
	s_maxFd = s_maxFd+1;
	

	printf("s_vencFd:%d, s_aencFd:%d, s_maxFd:%d,\n",s_vencFd,s_aencFd,s_maxFd);
	
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	
	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return 0;
	}
	unsigned int select_fail_count = 0; 
	while(cdr_mp4_checklen(g_mp4handle))
#if 1
	{
		FD_ZERO( &read_fds );
		FD_SET(s_vencFd, &read_fds);
		FD_SET(s_aencFd,&read_fds);

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			SAMPLE_PRT("select failed!\n");
			usleep(1000);
			select_fail_count ++; //modyfied: 2015-04-17.
			if (select_fail_count > 20)
			{
				printf("[%s, %d] ..........select failed count: %d ...........\n", __func__, __LINE__, select_fail_count);
				select_fail_count = 0;
			}
			else
				continue;
		}
		else if (s32Ret > 0)
		{
			select_fail_count = 0;
			if (FD_ISSET( s_vencFd, &read_fds ))
			{
				iLen = 0;
				s32Ret = HI_MPI_VENC_Query( s_vencChn, &stStat );
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
					usleep(1000);
					continue;
				}

				stStream.pstPack = pstPack;
				stStream.u32PackCount = stStat.u32CurPacks;
				s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stStream, HI_TRUE );
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_GetStream main.. failed with %#x!\n", s32Ret);
					usleep(1000);
					continue;
				}

				for (i = 0; i < stStream.u32PackCount; i++)
				{
					iLen = 0;
					memcpy( videobuff + iLen, stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset); 
					iLen += stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset;

					if(stStream.pstPack[i].DataType.enH264EType  == H264E_NALU_SEI ||
						stStream.pstPack[i].DataType.enH264EType  == H264E_NALU_SPS ||
						stStream.pstPack[i].DataType.enH264EType  == H264E_NALU_PPS) continue;//这儿把SEI帧去掉.暂时没啥用途.

					memset(pDataFrame,0x00,sizeof(MP4_FRAME));
					pDataFrame->streamType = MP4STREAM_VIDEO;
					pDataFrame->nFrameType = stStream.pstPack[i].DataType.enH264EType;
					pDataFrame->uPTS = stStream.pstPack[i].u64PTS;
					pDataFrame->nlen = iLen;	
					pDataFrame->pData= (unsigned char*)videobuff;		
					cdr_mp4_write_frame(g_mp4handle,pDataFrame);						
				}					

				s32Ret = HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
				if (HI_SUCCESS != s32Ret)
				{
					SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] main.. failed with %#x!\n", s_vencChn, s32Ret);
					stStream.pstPack = NULL;
					usleep(1000);
					continue;
				}
			}


			if (FD_ISSET( s_aencFd, &read_fds ))
			{
				//printf("write aac\n");
				/* get stream from aenc chn */
				s32Ret = HI_MPI_AENC_GetStream(s_aencChn, &stAStream, HI_FALSE);
				if (HI_SUCCESS != s32Ret )
				{
					printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
						   __FUNCTION__, s_aencChn, s32Ret);
				}	
				
				memcpy(videobuff,stAStream.pStream,stAStream.u32Len);

				memset(pDataFrame,0x00,sizeof(MP4_FRAME));
				pDataFrame->streamType = MP4STREAM_AUDIO;
				pDataFrame->nFrameType = 0;
				pDataFrame->uPTS = stAStream.u64TimeStamp;
				pDataFrame->nlen = stAStream.u32Len-7;	
				pDataFrame->pData= (unsigned char*)(videobuff+7);		
				cdr_mp4_write_frame(g_mp4handle,pDataFrame);			
				
				/* finally you must release the stream */
				s32Ret = HI_MPI_AENC_ReleaseStream(s_aencChn, &stAStream);
				if (HI_SUCCESS != s32Ret )
				{
					printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n",\
						   __FUNCTION__, s_aencChn, s32Ret);
				 }
		   }

		}
	}

	if(pstPack)  
		free(pstPack);

#else
	{
		usleep(30); 		

		pData = NULL;
		nSize = 0;
		memset(&vaFrame,0,sizeof(sVAFrameInfo));		
		StremPoolRead(ENUM_VS_REC, &pData,&nSize,&vaFrame);
		if((nSize > 4 && pData != NULL) && 
			(vaFrame.isIframe == H264E_NALU_ISLICE || vaFrame.isIframe == H264E_NALU_PSLICE ))
		{	
			memset(pDataFrame,0x00,sizeof(MP4_FRAME));
			pDataFrame->streamType = MP4STREAM_VIDEO;
			pDataFrame->nFrameType = vaFrame.isIframe;
			pDataFrame->uPTS = vaFrame.u64PTS;
			pDataFrame->nlen = nSize;	
			pDataFrame->pData= (unsigned char*)pData;		
			cdr_mp4_write_frame(g_mp4handle,pDataFrame);	

			if(uv64PTS == 0) uv64PTS = vaFrame.u64PTS;
		}	


		pData = NULL;
		nSize = 0;
		memset(&vaFrame,0,sizeof(sVAFrameInfo));		
		StremPoolRead(ENUM_AS_REC, &pData,&nSize,&vaFrame);

		if((nSize > 7 && pData != NULL))
		{	
			memset(pDataFrame,0x00,sizeof(MP4_FRAME));
			pDataFrame->streamType = MP4STREAM_AUDIO;
			pDataFrame->nFrameType = vaFrame.isIframe;
			pDataFrame->uPTS = vaFrame.u64PTS;
			pDataFrame->nlen = nSize-7; 
			pDataFrame->pData= (unsigned char*)pData+7; 	
			cdr_mp4_write_frame(g_mp4handle,pDataFrame);	
			
			if(ua64PTS == 0) ua64PTS = vaFrame.u64PTS;
		}
	

		if(ua64PTS>0 && uv64PTS>0 && flag == 0)
		{
			flag = 1;
			printf("----> a:%lld   v:%lld  sub:%lld \n",ua64PTS,uv64PTS,uv64PTS-ua64PTS);
		}
		//When power off. save mp4 to sd.
		if(0 == cdr_get_powerflag())
			break;		
	}
#endif
	cdr_mp4_close(g_mp4handle);

	free(pDataFrame);
	pDataFrame = NULL;
	
	return 0;
}


void cdr_video_index_file_synchronous(void)
{
	// 1.get first file of video.
	DIR *dir;
	struct dirent *ptr = NULL;
	dir = opendir("/mnt/mmc/VIDEO/");
	
	if(dir == NULL)
	{
		printf("%s cann't open dir :/mnt/mmc/VIDEO/ \n",__FUNCTION__);
		return;
	}
	
	char chFirstName[256];
	//这儿需要初始化为FF,保证第一个文件
	memset(chFirstName,0xFF,sizeof(chFirstName));
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;
		if(strcmp(ptr->d_name,chFirstName) < 0)
		{
			memset(chFirstName,0x00,256);
			//printf("%s \n",ptr->d_name);			
			snprintf(chFirstName, 256, "%s",ptr->d_name);		
		}	
	}
	closedir(dir);
	dir = NULL;

	//没有mp4文件.
	if(chFirstName[0] == 0xFF)
	{
		cdr_system("rm -rf /mnt/mmc/INDEX/*");
		sync();
		return ;
	}
	
	//printf("%s,%s\n",__FUNCTION__,chFirstName);		
	// 2. rm index pic before first video file.

	dir = opendir("/mnt/mmc/INDEX/");
	if(dir == NULL)
	{
		printf("%s cann't open dir :/mnt/mmc/INDEX/ \n",__FUNCTION__);
		return;
	}
	
	char chIndexTmp[256];
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)
			continue;

		//printf("%s < %s\n",ptr->d_name,chFirstName);
		if(strcmp(ptr->d_name,chFirstName) < 0)
		{
			//printf("rm %s 99999999999999999999\n",ptr->d_name);
			memset(chIndexTmp,0x00,256);
			snprintf(chIndexTmp, 256, "/mnt/mmc/INDEX/%s",ptr->d_name);	
			remove(chIndexTmp);
		}	
	}
	closedir(dir);
	sync();
}
