#ifndef _CDR_VEDIO_H_
#define _CDR_VEDIO_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "stream_pool.h"
#include "cdr_writemov.h"
#include "sample_comm.h"


typedef struct venc_getstream_ch_s
{
     int bThreadStart;
	 int  s32Cnt;
     int  s32VencChn;
	 int  framerate;
	 unsigned char  nsnapflag;
 	 unsigned char  exsnapflag;//snap 快照 ，对应不同位置的照片
}VENC_GETSTREAM_CH_PARA_S;


/******************************************************************************
* function :  H.264@720p@30fps+H.264@VGA@30fps+H.264@QVGA@30fps
******************************************************************************/
HI_S32 sub_video_init(HI_VOID);

/******************************************************************************
* function    : cdr_vedioInit()
* Description : init vedio.
******************************************************************************/
int cdr_videoInit(HI_VOID);

/******************************************************************************
* function    : sub_vedio_realase()
* Description : video realase cfg.
******************************************************************************/
void cdr_video_release(HI_VOID);


HI_S32 venc_start_getstream(int s32Cnt);


int cdr_isp_init(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //_CDR_VEDIO_H_