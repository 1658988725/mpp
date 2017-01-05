/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include "cdr_mpp.h"
#include "rtspsrvffmpg.h"
#include "cdr_app_service.h"
#include "cdr_device_discover.h"
#include "rtspd_api.h"
#include "cdr_XmlLog.h"
#include "cdr_mp4_api.h"

#define CDR_RTSP_PORT 554

int main(int argc, char *argv[])
{	
	HI_S32 s32Ret = HI_TRUE;
	printf("Software Compiled Time: %s %s\r\n",__DATE__, __TIME__);
	signal(SIGPIPE, SIG_IGN);
	cdr_config_init();
	cdr_mpp_init();		
	cdr_init_record();
    cdr_device_init();    

	cdr_system_wait();
	cdr_device_deinit();	
    exit(s32Ret);
}

