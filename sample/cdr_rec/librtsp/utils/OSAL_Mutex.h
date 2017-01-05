
#ifndef __OSAL_MUTEX__
#define __OSAL_MUTEX__

#ifdef __cplusplus
extern "C" {
#endif

#include "OSAL_Common.h"

OMX_ERRORTYPE OSAL_MutexCreate(OMX_HANDLETYPE *mutexHandle);
OMX_ERRORTYPE OSAL_MutexTerminate(OMX_HANDLETYPE mutexHandle);
OMX_ERRORTYPE OSAL_MutexLock(OMX_HANDLETYPE mutexHandle);
OMX_ERRORTYPE OSAL_MutexUnlock(OMX_HANDLETYPE mutexHandle);

#ifdef __cplusplus
}
#endif


#endif

