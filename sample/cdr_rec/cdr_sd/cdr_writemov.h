#ifndef _CDR_WRITEMOV_H
#define _CDR_WRITEMOV_H

/************************************************************************
//cdr_writemov.h
//Read Buffer from Stream pool.and write stream to MP4
*************************************************************************/

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

typedef struct sd_param
{
    short audio;
    short moveRec;
    short outMoveRec;
    short autoRec;
    short loopWrite;
    short splite;
    short sdrecqc;//1 one, 0 two
    short sdIoOpen;//1 open, 0 close
    short sdError;//1 open, 0 close
    short sdMoveOpen;//0 open, 1 close
    unsigned long allSize;
    unsigned long haveUse;
    unsigned long leftSize;

    //    iVideoSize uiProtectSize uiPhotoSize uiOtherSize
    unsigned long iVideoSize;
    unsigned long uiProtectSize;
    unsigned long uiPhotoSize; 
    unsigned long uiOtherSize; 
    
}SD_PARAM;

extern short g_TF_flag;
extern SD_PARAM g_SdParam;
extern int g_sdIsOnline;
extern int g_mp4handle;


int cdr_init_record(void);
int GetStorageInfo(void);
void cdr_unload_sd(void);

#endif//_CDR_WRITEMOV_H

