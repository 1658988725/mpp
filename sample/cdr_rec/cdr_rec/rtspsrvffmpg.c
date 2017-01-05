#include "rtspsrvffmpg.h"
//#include "parsemp4file.h"
#include "cdr_rtsp_server.h"
#include "cdr_rec.h"
#include "cdr_mp4_api.h"

int g_pauseflag = 1;
int g_playflag = 0;

int g_seekflag = 0;
unsigned long g_seekTimeDS = 0;
int g_framenum = 0;
struct timeval g_oplaystarttm;
unsigned int g_nframerate;
RTPSendBuf g_oRTPSendBuf;


int rec_mp4_read_pro(void)
{
	char *pData = malloc(1920*1080);
	int len;
	int iFrame;
	while (g_rec_flag)
	{
		if(g_rec_pasueflag == 1)
		{
			usleep(300);
			continue;
		}
		len = 0;
		iFrame = 0;
		memset(pData,0x00,sizeof(pData));
		if(g_rec_data_callbak != NULL && 0 == cdr_mp4ex_read_vframe(&pData,&len,&iFrame))
		{	
		
			if(iFrame == CDR_H264_NALU_ISLICE)
				iFrame = 1;
			else
				iFrame = 0;
			
			g_rec_data_callbak(STREAM_VIDEO,pData,len,iFrame);
		}	

		usleep(5);	
	}
	free(pData);
	
	return 0;
}


//Read from sd.
int cdr_rec_mp4_read(void)
{
	int ret = -1;
	pthread_t tfid;
	ret = pthread_create(&tfid, NULL, (void *)rec_mp4_read_pro, NULL);
	if (0 != ret)
	{
		//DEBUG_PRT("create TF record thread failed!\n");
		return -1;
	}
	return 0;
}

