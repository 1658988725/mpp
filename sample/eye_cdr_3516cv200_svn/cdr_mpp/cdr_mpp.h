#ifndef _CDR_MPP_H_
#define _CDR_MPP_H_

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
#include "cdr_audio.h"
#include "stream_pool.h"
#include "cdr_writemov.h"
#include "sample_comm.h"
#include "acodec.h"
#include "cdr_vedio.h"

enum CDR_FUNCALLBAK_TYPE
{
	CDR_REC_TYPE     = 0,
	CDR_LIVE_TYPE    = 1,
	CDR_SNAP_TYPE    = 2,
	CDR_INDEX_TYPE   = 3,
	CDR_CALLBACK_MAX = 4	
};

#define STREAM_VIDEO 0
#define STREAM_AUDIO 1

typedef int (*cdr_stream_callback)(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame);
extern VENC_GETSTREAM_CH_PARA_S gsjpeg_stPara;
extern VENC_GETSTREAM_CH_PARA_S gsqcif_stPara;

int cdr_mpp_init(void);
int cdr_mpp_release(void);
void cdr_stream_setevent_callbak(int ntype,cdr_stream_callback callback);
void cdr_capture_jpg(int type);
void cdr_capture_qcif(int type);

int GetFullJpgName(char *pDst);
#endif 