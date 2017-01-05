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

    set_cdr_start_time();    

	cdr_mpp_init();		

	//Init sd.and begin rec....
	cdr_init_record();//录相
	
	cdr_wifi_init();
	
	cdr_init_mp4dir(MP4DIRPATH);
	
	//init protocol.
	cdr_app_service_init();
	
	cdr_device_discover_init();

	cdr_rtsp_init(CDR_RTSP_PORT);
	
	cdr_rec_rtsp_init(); //回放 rtsp
	cdr_live_rtsp_init();//直播 rtsp

    cdr_device_init();    

	//Wait for finish.
	cdr_system_wait();

	//wait save all to SD.	
	cdr_device_deinit();	

	while(1)
	{
		//the second time system power up.
		if(cdr_get_powerflag())
		{
			printf("reboot system\n");
			cdr_system("reboot");
	     }
		printf("cdr wait for end..\r\n");
		usleep(1000000);
	}
    exit(s32Ret);

}

