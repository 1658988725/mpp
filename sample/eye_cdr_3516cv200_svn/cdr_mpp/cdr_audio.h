#ifndef _CDR_AUDIO_H_
#define _CDR_AUDIO_H_

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
//#include "acodec.h"
//#include "aacenc.h"
//#include "aacdec.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

HI_S32 cdr_audioInit(void);

HI_S32 cdr_audio_release(void);

int cdr_audio_play_file(char *cAudioFile);
HI_S32 Video_SubSystem_Init();

int SetVodieFlip(unsigned char mirFlg,unsigned char flipFlg,VPSS_CHN VpssChn,int enSizeIndex);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif //_CDR_AUDIO_H_