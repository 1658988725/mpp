/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
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
#include "cdr_vedio.h"
#include "cdr_audio.h"
#include "cdr_mpp.h"
#include "osd_region.h"

#include "cdr_config.h"

#define TYPE_1080P 0x02
#define TYPE_720P  0x01
#define TYPE_480P  0x00

#define MODE_16_9   0x00
#define MODE_4_3    0x01

#define ROTATE_POSITIVE  0x00
#define ROTATE_INVERTED  0x01
#define ROTATE_MIRROR    0x02

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;


#ifdef hi3518ev201
HI_U32 g_u32BlkCnt = 4;
#endif

#ifdef hi3518ev200
HI_U32 g_u32BlkCnt = 4;
#endif

#ifdef hi3516cv200
HI_U32 g_u32BlkCnt = 10;
#endif


static VENC_GETSTREAM_CH_PARA_S gsrec_stPara;
static VENC_GETSTREAM_CH_PARA_S gslive_stPara;


static pthread_t gsrec_VencPid;
static pthread_t gslive_VencPid;


/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{

    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

#if(0)
//四个通道.
HI_S32 Video_SubSystem_Init(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[4]= {PT_H264, PT_H264,PT_JPEG,PT_JPEG};
    PIC_SIZE_E enSize[4] = {PIC_HD1080, PIC_VGA,PIC_HD1080,PIC_QCIF};
	HI_U32 u32Profile = 0;	
    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;	
    HI_S32 s32ChnNum=4;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
	printf("s32ChnNum = %d\n",s32ChnNum);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =g_u32BlkCnt;
	}
	if(s32ChnNum >= 3)
    {
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[2].u32BlkCnt = g_u32BlkCnt;
    }
	if(s32ChnNum >= 4)
    {
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[3], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[3].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[3].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Vi bind Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_3;
	    }

		VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = 30;
	    stVpssChnAttr.s32DstFrameRate = 30;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	if(s32ChnNum >= 2)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = 30;
	    stVpssChnAttr.s32DstFrameRate = 30;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
	

	if(s32ChnNum >= 3)
	{	
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		VpssChn = 2;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnMode.u32Width		= stSize.u32Width;
		stVpssChnMode.u32Height 	= stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}


	
	if(s32ChnNum >= 4)
	{	
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[3], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		VpssChn = 3;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnMode.u32Width		= stSize.u32Width;
		stVpssChnMode.u32Height 	= stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	
    /******************************************
     step 5: start stream venc
    ******************************************/

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                    gs_enNorm, enSize[1], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;

		SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);		

		s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start snap failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}


	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[3] **/
	if(s32ChnNum >= 4)
	{
	    VpssChn = 3;
	    VencChn = 3;

		SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[3], &stSize);	
		s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start snap failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	printf("%s OK \r\n",__FUNCTION__);
	return 0;

END_VENC_1080P_CLASSIC_5:	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;   
		    VencChn = 3;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 3:
			VpssChn = 2;   
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	
END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;
	
	}

END_VENC_1080P_CLASSIC_3:    //vpss stop       
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

	return 0;
}
#else

SAMPLE_VI_CONFIG_S stViConfig = {0};
VPSS_GRP VpssGrp;
VPSS_GRP_ATTR_S stVpssGrpAttr;
VPSS_CHN VpssChn;
VPSS_CHN_ATTR_S stVpssChnAttr;
VPSS_CHN_MODE_S stVpssChnMode;
//四个通道.
HI_S32 Video_SubSystem_Init()
{
    PAYLOAD_TYPE_E enPayLoad[4]= {PT_H264, PT_H264,PT_JPEG,PT_JPEG};
    PIC_SIZE_E enSize[4] = {PIC_HD1080, PIC_VGA,PIC_HD1080,PIC_QCIF};
	HI_U32 u32Profile = 0;	
    VB_CONF_S stVbConf;

    
    //VPSS_GRP VpssGrp;
    //VPSS_CHN VpssChn;
    //VPSS_GRP_ATTR_S stVpssGrpAttr;
    //VPSS_CHN_ATTR_S stVpssChnAttr;
    //VPSS_CHN_MODE_S stVpssChnMode;
    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;	
    HI_S32 s32ChnNum = 4;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    //char c;

    unsigned char rotate = g_cdr_systemconfig.sCdrVideoRecord.rotate;
    //unsigned char ucType = g_cdr_systemconfig.sCdrVideoRecord.type;//分辨率

#if(0)    
    if(ucType = 0x00)//480
    {
      enSize[0] = PIC_VGA;
      enSize[1] = PIC_VGA;
      enSize[2] = PIC_HD1080;
      enSize[3] = PIC_QCIF;
    }
    if(ucType = 0x01)//720
    {
      enSize[0] = PIC_HD720;
      enSize[1] = PIC_VGA;
      enSize[2] = PIC_HD1080;
      enSize[3] = PIC_QCIF;
    }
    if(ucType = 0x02)//1080
    {
      enSize[0] = PIC_HD1080;
      enSize[1] = PIC_VGA;
      enSize[2] = PIC_HD1080;
      enSize[3] = PIC_QCIF;
    }
#endif

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
	//printf("s32ChnNum = %d\n",s32ChnNum);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    int i = 0;
    for(i = 0;i < 4;i++)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[i], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[i].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[i].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;

    stViConfig.enRotate   = ROTATE_NONE;
        
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
	if(s32ChnNum >= 1)//record
	{
		VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;
	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Vi bind Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_3;
	    }

		VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = 30;
	    stVpssChnAttr.s32DstFrameRate = 30;

         stVpssChnAttr.bMirror = 1;
         
         stVpssChnAttr.bFlip = 1;
         
         if(rotate == 0x02) stVpssChnAttr.bMirror = 0;//是否水平翻转				
         if(rotate == 0x01)stVpssChnAttr.bFlip = 0;//是否垂直翻转

         
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
#if(0)
        /*
        图像矫正
        bEnable LDC 使能开关。
        stAttr enViewType 畸变校正类型，裁剪模式、全模式。
        s32CenterXOffset 畸变中心点相对图象中心点水平偏移 [-75, 75]。
        s32CenterYOffset 畸变中心点相对图象中心点垂直偏移 [-75, 75]。
        s32Ratio 畸变程度 [0, 511]。
        */       
        VPSS_LDC_ATTR_S *pstLDCAttr;
        pstLDCAttr->bEnable = 0x01;
        pstLDCAttr->stAttr.enViewType = LDC_VIEW_TYPE_ALL;
        pstLDCAttr->stAttr.s32CenterXOffset = 0;
        pstLDCAttr->stAttr.s32CenterYOffset = 0;
        pstLDCAttr->stAttr.s32Ratio = 255;
        HI_MPI_VPSS_SetLDCAttr(VpssGrp,VpssChn,pstLDCAttr);
#endif        
	}

	if(s32ChnNum >= 2)//live
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = 30;
	    stVpssChnAttr.s32DstFrameRate = 30;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;

         if(rotate == 0x02) stVpssChnAttr.bMirror = 0;//是否水平翻转				
         if(rotate == 0x01)stVpssChnAttr.bFlip = 0;//是否垂直翻转
                 
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
	

	if(s32ChnNum >= 3)
	{	
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		VpssChn = 2;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnMode.u32Width		= stSize.u32Width;
		stVpssChnMode.u32Height 	= stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;

         if(rotate == 0x02) stVpssChnAttr.bMirror = 0;//是否水平翻转				
         if(rotate == 0x01)stVpssChnAttr.bFlip = 0;//是否垂直翻转        
         
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}


	
	if(s32ChnNum >= 4)
	{	
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[3], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		VpssChn = 3;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnMode.u32Width		= stSize.u32Width;
		stVpssChnMode.u32Height 	= stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bMirror = 1;
		stVpssChnAttr.bFlip = 1;

         if(rotate == 0x02) stVpssChnAttr.bMirror = 0;//是否水平翻转				
         if(rotate == 0x01)stVpssChnAttr.bFlip = 0;//是否垂直翻转        
		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	
    /******************************************
     step 5: start stream venc
    ******************************************/

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                    gs_enNorm, enSize[1], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;

		SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);		

		s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start snap failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}


	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[3] **/
	if(s32ChnNum >= 4)
	{
	    VpssChn = 3;
	    VencChn = 3;

		SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[3], &stSize);	
		s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start snap failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	printf("%s OK \r\n",__FUNCTION__);
	return 0;

END_VENC_1080P_CLASSIC_5:	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;   
		    VencChn = 3;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 3:
			VpssChn = 2;   
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	
END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;
	
	}

END_VENC_1080P_CLASSIC_3:    //UnBindVpss stop       
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

	return 0;
}
#endif

/*
0:翻转
1:正像
*/
int SetVodieFlip(unsigned char mirFlg,unsigned char flipFlg,VPSS_CHN VpssChn,int enSizeIndex)
{
    int s32Ret = 0;
    SIZE_S stSize;
    PIC_SIZE_E enSize[4] = {PIC_HD1080, PIC_VGA,PIC_HD1080,PIC_QCIF};
    
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    s32Ret = HI_MPI_VPSS_GetChnAttr(VpssGrp,VpssChn,&stVpssChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
    SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
    return HI_FAILURE;
    }
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp,VpssChn);
    if(s32Ret != HI_SUCCESS)
    {
    SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
    return HI_FAILURE;
    }
    
    /*vpss chn 1:video stream*/
    //s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm,enSize[1], &stSize);
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm,enSize[enSizeIndex], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
    SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
    return HI_FAILURE;
    }

    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;/*HI3516A only support 'USER' mode*/
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width       = stSize.u32Width;
    stVpssChnMode.u32Height      = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    if(mirFlg==0x00 || mirFlg==0x01) stVpssChnAttr.bMirror = mirFlg;//设置当前的镜像标志 参数传进来的
    if(flipFlg==0x00 || flipFlg==0x01) stVpssChnAttr.bFlip = flipFlg;//设置垂直翻转标志，参数传递进来的

    stVpssChnAttr.s32SrcFrameRate = 30;/*range: -1 - 60*//*-1:donot set framerate*/
    stVpssChnAttr.s32DstFrameRate = 30;/*hi3516a support 25 and 30*/

    /*enable vpss channel */
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
     SAMPLE_PRT("Enable vpss chn failed!\n");
     return HI_FAILURE;
    }
    
    return 0;
}



int cdr_mpp_stop()
{
    int i = 0;
    //UnBindVpss  VENC_Stop
    for(i=0;i<4;i++){
	  SAMPLE_COMM_VENC_UnBindVpss(i, 0, i);
	  SAMPLE_COMM_VENC_Stop(i);
    }    

    //VPSS_DisableChn
    for(i=0;i<4;i++){
      SAMPLE_COMM_VPSS_DisableChn(0, i);
    }
    //unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
    //Group stop   
    SAMPLE_COMM_VPSS_StopGroup(0);
    //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
}



int cdr_mpp_set_video_mode(char cIndex)
{
#if 0    //待修正
   if((g_cdr_systemconfig.sCdrVideoRecord.mode == 0x00)&&(cIndex == 0x01))
   {
       OSD_Overlay_RGN_Handle_Init1(6, 2,0x01,0x01);
       OSD_Overlay_RGN_Handle_Init1(7, 2,0x01,0x02);

       OSD_Overlay_RGN_Handle_Init1(8, 0,0x01,0x01);
       OSD_Overlay_RGN_Handle_Init1(9, 0,0x01,0x02);
   }
   //0:record 通道 2 jpg
   if((g_cdr_systemconfig.sCdrVideoRecord.mode == 0x01)&&(cIndex == 0x00))
   {
       OSD_Overlay_RGN_Handle_Init1(6, 2,0x00,0x01);
       OSD_Overlay_RGN_Handle_Init1(7, 2,0x00,0x02);

       OSD_Overlay_RGN_Handle_Init1(8, 0,0x00,0x01);
       OSD_Overlay_RGN_Handle_Init1(9, 0,0x00,0x02);
   }
#endif   
   g_cdr_systemconfig.sCdrVideoRecord.mode= cIndex;
   
   return 0;
}

/*
rotate	图像旋转	0	0	正像
			1	倒像
			2	镜像
*/
int cdr_mpp_set_video_rotate(char cIndex)
{
    int i;
    g_cdr_systemconfig.sCdrVideoRecord.rotate = cIndex ;   

    for(i=0;i<4;i++){
       if(cIndex == 0x00)SetVodieFlip(1,1,i,i);  //正像 倒像 镜像回正   
       if(cIndex == 0x01)SetVodieFlip(3,0,i,i);  //倒像
       if(cIndex == 0x02)SetVodieFlip(0,3,i,i);  //镜像
    }
       
    return 0;     
}



int cdr_mpp_set_video_resolution(char cIndex)
{
    //cdr_mpp_stop();
    g_cdr_systemconfig.sCdrVideoRecord.type = cIndex;
    return 0;
}

int cdr_printf_gama(void)
{
	HI_S32 s32Ret = 0;
	ISP_GAMMA_ATTR_S stGammaAttr;
	s32Ret =  HI_MPI_ISP_GetGammaAttr(0,&stGammaAttr);
	
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetH264Dblk failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	printf("stGammaAttr.bEnable:%d\n",stGammaAttr.bEnable);
	printf("stGammaAttr.enCurveType:%d\n",stGammaAttr.enCurveType);

	
	int i=0;
	for(i=0;i<GAMMA_NODE_NUM;i++)
	{
			printf("%d,",stGammaAttr.u16Table[i]);
			if(i>0 && i%16 == 0)	printf("\n");
	}
	return 0;
	
}

int isp_set_gama(void)
{
	return 0;
	HI_S32 s32Ret = 0;
	ISP_GAMMA_ATTR_S stGammaAttr;

	int iGama = 1;
	s32Ret =  HI_MPI_ISP_GetGammaAttr(0,&stGammaAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetH264Dblk failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	if(iGama == 0)
	{
		unsigned short sGama[GAMMA_NODE_NUM] = 
		{
			0,120,220,310,390,470,540,610,670,730,786,842,894,944,994,1050,1096,
			1138,1178,1218,1254,1280,1314,1346,1378,1408,1438,1467,1493,1519,1543,1568,1592,
			1615,1638,1661,1683,1705,1726,1748,1769,1789,1810,1830,1849,1869,1888,1907,1926,
			1945,1963,1981,1999,2017,2034,2052,2069,2086,2102,2119,2136,2152,2168,2184,2200,
			2216,2231,2247,2262,2277,2292,2307,2322,2337,2351,2366,2380,2394,2408,2422,2436,
			2450,2464,2477,2491,2504,2518,2531,2544,2557,2570,2583,2596,2609,2621,2634,2646,
			2659,2671,2683,2696,2708,2720,2732,2744,2756,2767,2779,2791,2802,2814,2825,2837,
			2848,2859,2871,2882,2893,2904,2915,2926,2937,2948,2959,2969,2980,2991,3001,3012,
			3023,3033,3043,3054,3064,3074,3085,3095,3105,3115,3125,3135,3145,3155,3165,3175,
			3185,3194,3204,3214,3224,3233,3243,3252,3262,3271,3281,3290,3300,3309,3318,3327,
			3336,3346,3356,3365,3375,3385,3395,3404,3414,3423,3432,3441,3449,3457,3464,3471,
			3477,3483,3488,3493,3497,3501,3505,3509,3512,3515,3519,3522,3526,3530,3534,3539,
			3544,3549,3554,3560,3565,3571,3576,3582,3587,3593,3599,3605,3610,3616,3621,3627,
			3633,3638,3644,3650,3656,3663,3669,3675,3681,3686,3692,3697,3702,3707,3711,3715,
			3718,3721,3724,3726,3727,3729,3730,3731,3732,3734,3735,3736,3738,3739,3741,3744,
			3747,3750,3753,3756,3760,3764,3767,3771,3775,3779,3783,3787,3791,3795,3798,3802,

		};
		memcpy(stGammaAttr.u16Table,sGama,GAMMA_NODE_NUM*sizeof(unsigned short));	
	}
	else if(iGama == 1)
	{
		unsigned short sGama[GAMMA_NODE_NUM] = 
		{
			0,120,220,310,390,470,540,610,670,730,786,842,894,944,994,1050,1096,
			1138,1178,1218,1254,1280,1314,1346,1378,1408,1438,1467,1493,1519,1543,1568,1592,
			1615,1638,1661,1683,1705,1726,1748,1769,1789,1810,1830,1849,1869,1888,1907,1926,
			1945,1964,1983,2001,2020,2038,2056,2073,2091,2108,2124,2140,2156,2171,2186,2200,
			2213,2226,2239,2250,2261,2272,2282,2293,2302,2312,2321,2331,2340,2350,2359,2369,
			2379,2389,2399,2409,2419,2429,2439,2449,2458,2468,2477,2485,2493,2501,2508,2515,
			2521,2527,2531,2536,2540,2544,2547,2550,2553,2556,2559,2561,2564,2567,2570,2574,
			2578,2581,2585,2589,2593,2597,2601,2605,2609,2612,2616,2619,2623,2626,2629,2632,
			2635,2638,2640,2642,2644,2646,2648,2650,2652,2654,2656,2659,2662,2680,2702,2720,
			2723,2725,2727,2729,2731,2733,2735,2736,2738,2739,2741,2742,2744,2746,2747,2749,
			2751,2753,2755,2756,2758,2760,2762,2764,2766,2768,2770,2772,2773,2775,2777,2779,
			2781,2783,2784,2786,2788,2790,2792,2794,2795,2797,2799,2801,2803,2804,2806,2808,
			2810,2812,2813,2815,2817,2819,2821,2823,2824,2826,2828,2830,2832,2833,2835,2837,
			2839,2841,2843,2846,2848,2850,2853,2855,2857,2859,2861,2863,2864,2865,2866,2866,
			2866,2865,2864,2863,2861,2859,2857,2855,2853,2850,2848,2846,2843,2841,2839,2837,
			2834,2831,2828,2825,2822,2819,2816,2813,2811,2809,2808,2808,2811,2819,2828,2837,
		};
		memcpy(stGammaAttr.u16Table,sGama,GAMMA_NODE_NUM*sizeof(unsigned short));	
	}
	stGammaAttr.enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
	s32Ret =  HI_MPI_ISP_SetGammaAttr(0,&stGammaAttr);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_VENC_GetH264Dblk failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}
	
	return HI_SUCCESS;
}

//ExposureAttr
int isp_set_ExposureAttr(void)
#if 1
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
#if 0	
	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_AUTO;
	stExpAttr.stAuto.stExpTimeRange.u32Max = 30000;
	stExpAttr.stAuto.stExpTimeRange.u32Min = 10;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	
	stExpAttr.stAuto.u8Speed = 0x40;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
#endif	
	
	stExpAttr.stAuto.stAGainRange.u32Min = 1024;
	stExpAttr.stAuto.stAGainRange.u32Max= 4096;

	stExpAttr.stAuto.stDGainRange.u32Min = 1024;
	stExpAttr.stAuto.stDGainRange.u32Max = 2048;

	stExpAttr.stAuto.stISPDGainRange.u32Min = 1024;
	stExpAttr.stAuto.stISPDGainRange.u32Max = 2048;
	
	stExpAttr.stAuto.stSysGainRange.u32Min = 1024;
	stExpAttr.stAuto.stSysGainRange.u32Max = 8192;

	stExpAttr.stAuto.u16EVBias = 900;
	//stExpAttr.stAuto.u8Speed = 64;
		
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);


	
#if 0
	
		HI_U8 i,j;
		HI_U8 u8Weighttable[AE_ZONE_ROW][AE_ZONE_COLUMN]=
		{	
			0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,0x81,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x81,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
			
			0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,0x81,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


		};
		for (i = 0; i < AE_ZONE_ROW; i++)
		{
			for (j = 0; j < AE_ZONE_COLUMN; j++)
			{
				stExpAttr.stAuto.au8Weight[i][j] = u8Weighttable[i][j];
			}
		}
		HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
#endif

#if 0	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.stAuto.stAntiflicker.bEnable = HI_TRUE;
	stExpAttr.stAuto.stAntiflicker.u8Frequency = 50;
	//stExpAttr.stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_NORMAL_MODE;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
#endif

#if 0
	//饱和度
	ISP_SATURATION_ATTR_S stSatAttr;
	HI_MPI_ISP_GetSaturationAttr(IspDev,&stSatAttr);
	stSatAttr.enOpType = OP_TYPE_MANUAL;
	stSatAttr.stManual.u8Saturation = 140;
	HI_MPI_ISP_SetSaturationAttr(IspDev,&stSatAttr);
#endif

	return 0;	
}

#else
{
	ISP_DEV IspDev = 0;
	ISP_EXPOSURE_ATTR_S stExpAttr;
	HI_MPI_ISP_GetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.bByPass = HI_FALSE;
	stExpAttr.enOpType = OP_TYPE_AUTO;
	stExpAttr.stAuto.stExpTimeRange.u32Max = 30000;
	stExpAttr.stAuto.stExpTimeRange.u32Min = 10;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	
	stExpAttr.stAuto.u8Speed = 0x40;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	
	stExpAttr.stAuto.stSysGainRange.u32Min = 1024;
	stExpAttr.stAuto.stSysGainRange.u32Max= 20240;

	stExpAttr.stAuto.stDGainRange.u32Min = 1024;
	stExpAttr.stAuto.stDGainRange.u32Max = 1024;

	stExpAttr.stAuto.u16EVBias = 800;
	stExpAttr.stAuto.u8Speed = 128;
		
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	
	//HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	//stExpAttr.stAuto.enAEStrategyMode = AE_EXP_LOWLIGHT_PRIOR;
	//stExpAttr.stAuto.u16HistRatioSlope = 0x100;
	//stExpAttr.stAuto.u8MaxHistOffset = 0x40;
	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.stAuto.stAntiflicker.bEnable = HI_TRUE;
	stExpAttr.stAuto.stAntiflicker.u8Frequency = 50;
	//stExpAttr.stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_NORMAL_MODE;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	
	//饱和度
	ISP_SATURATION_ATTR_S stSatAttr;
	HI_MPI_ISP_GetSaturationAttr(IspDev,&stSatAttr);
	stSatAttr.enOpType = OP_TYPE_MANUAL;
	stSatAttr.stManual.u8Saturation = 150;
	HI_MPI_ISP_SetSaturationAttr(IspDev,&stSatAttr);


#if 0	
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
	stExpAttr.stAuto.stAEDelayAttr.u16BlackDelayFrame = 10;
	stExpAttr.stAuto.stAEDelayAttr.u16WhiteDelayFrame = 0;
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);

	HI_U8 i,j;
	HI_U8 u8Weighttable[AE_ZONE_ROW][AE_ZONE_COLUMN]=
	{	
#if 0	
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,		
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1		
#endif
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC,0x0,0x0,0x0,	
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0

	};
	for (i = 0; i < AE_ZONE_ROW; i++)
	{
		for (j = 0; j < AE_ZONE_COLUMN; j++)
		{
			stExpAttr.stAuto.au8Weight[i][j] = u8Weighttable[i][j];
		}
	}
	HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
#endif

	return 0;	
}
#endif

//open dcr.
int isp_set_dcr(void)
{
	ISP_DEV IspDev = 0;
	ISP_DRC_ATTR_S stDRC;
	HI_MPI_ISP_GetDRCAttr(IspDev,&stDRC);

	stDRC.bEnable = HI_TRUE;
	stDRC.enOpType = OP_TYPE_AUTO;
	HI_MPI_ISP_SetDRCAttr(IspDev,&stDRC);
	return 0;
}

//HI_MPI_ISP_GetDefogAttr
int isp_set_defog(void)
{	
	ISP_DEV IspDev = 0;
	ISP_DEFOG_ATTR_S stDefogAttr;
	HI_MPI_ISP_GetDeFogAttr(IspDev,&stDefogAttr);
	stDefogAttr.bEnable = HI_TRUE;
	stDefogAttr.u8HorizontalBlock = 6;
	stDefogAttr.u8VerticalBlock = 6;
	stDefogAttr.enOpType = OP_TYPE_AUTO;
	stDefogAttr.stAuto.u8strength = 0x80;
	HI_MPI_ISP_SetDeFogAttr(IspDev,&stDefogAttr);	
	return 0;
}


int cdr_isp_init(void)
{
	isp_set_ExposureAttr();
	//isp_set_dcr();
	//isp_set_defog();
	isp_set_gama();
#if 0
	ISP_WDR_MODE_S stWDRMode;
	HI_MPI_ISP_GetWDRMode(0,&stWDRMode);
	printf("stWDRMode.enWDRMode:%d\n",stWDRMode.enWDRMode);
	stWDRMode.enWDRMode = WDR_MODE_BUILT_IN;
	HI_MPI_ISP_SetWDRMode(0,&stWDRMode);
#endif	
}

/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int cdr_videoInit()
{
	HI_S32 s32Ret;

	signal(SIGINT, SAMPLE_VENC_HandleSig);
	signal(SIGTERM, SAMPLE_VENC_HandleSig);

     //video init

     //g_cdr_systemconfig.cdrvideoRecordCfg.rotate;
	//s32Ret = Video_SubSystem_Init(TYPE_720P,MODE_16_9,ROTATE_POSITIVE);

     s32Ret = Video_SubSystem_Init();
	
	/**overlay region init**/
    cdr_OsdInit();

	//时间水印线程.
	start_region_update_thread();
	
	return s32Ret;
}


void cdr_video_release(void)
{
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;    
    VENC_CHN VencChn;	
    HI_S32 s32ChnNum=4;

	SAMPLE_VI_CONFIG_S stViConfig = {0};
	stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;	

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;   
		    VencChn = 3;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 3:
			VpssChn = 2;   
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	
	//vpss stop
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 4:
			VpssChn = 3;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;
	
	}
    //vpss stop       
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
	//system exit
    SAMPLE_COMM_SYS_Exit();
}

#if(0)
HI_VOID* VENC_GetVencStreamProc(HI_VOID *p)
{
	
    HI_S32 i;
    HI_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_GETSTREAM_CH_PARA_S *pstPara;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd[VENC_MAX_CHN_NUM];
    HI_CHAR aszFileName[VENC_MAX_CHN_NUM][64];
    FILE *pFile[VENC_MAX_CHN_NUM];
    char szFilePostfix[10];
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoadType[VENC_MAX_CHN_NUM];
    RGN_HANDLE RgnHandle = 0;
    pstPara = (SAMPLE_VENC_GETSTREAM_PARA_S*)p;
    s32ChnTotal = pstPara->s32Cnt;
	i = pstPara->s32VencChn;

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        SAMPLE_PRT("input count invaild\n");
        return NULL;
    }
	SAMPLE_PRT("s32ChnTotal:%d\r\n",s32ChnTotal);

    //for (i = 0; i < s32ChnTotal; i++)
    {
        /* decide the stream file name, and open file to save stream */
        VencChn = i;
        s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
                   VencChn, s32Ret);
            return NULL;
        }
        enPayLoadType[i] = stVencChnAttr.stVeAttr.enType;

        s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType[i], szFilePostfix);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", \
                   stVencChnAttr.stVeAttr.enType, s32Ret);
            return NULL;
        }
#if WRITE_H264_FILE		
        sprintf(aszFileName[i], "stream_chn%d%s", i, szFilePostfix);
        pFile[i] = fopen(aszFileName[i], "wb");
        if (!pFile[i])
        {
            SAMPLE_PRT("open file[%s] failed!\n", 
                   aszFileName[i]);
            return NULL;
        }
#endif		

        /* Set Venc Fd. */
        VencFd[i] = HI_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            SAMPLE_PRT("HI_MPI_VENC_GetFd failed with %#x!\n", 
                   VencFd[i]);
            return NULL;
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (HI_TRUE == pstPara->bThreadStart)
    {
    	//usleep(1);
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            SAMPLE_PRT("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
            for (i = 0; i < s32ChnTotal; i++)
             {
				if (i == pstPara->s32VencChn && FD_ISSET(VencFd[i], &read_fds))
				{
					//usleep(1);
				    /*******************************************************
				     step 2.1 : query how many packs in one-frame stream.
				    *******************************************************/
				    memset(&stStream, 0, sizeof(stStream));
				    s32Ret = HI_MPI_VENC_Query(i, &stStat);
				    if (HI_SUCCESS != s32Ret)
				    {
				        SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", i, s32Ret);
				        break;
				    }

				    /*******************************************************
				     step 2.2 : malloc corresponding number of pack nodes.
				    *******************************************************/
				    stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
				    if (NULL == stStream.pstPack)
				    {
				        SAMPLE_PRT("malloc stream pack failed!\n");
				        break;
				    }
				    
				    /*******************************************************
				     step 2.3 : call mpi to get one-frame stream
				    *******************************************************/
				    stStream.u32PackCount = stStat.u32CurPacks;
				    s32Ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
				    if (HI_SUCCESS != s32Ret)
				    {
				        free(stStream.pstPack);
				        stStream.pstPack = NULL;
				        SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", \
				               s32Ret);
				        break;
				    }

				    /*******************************************************
				     step 2.4 : save frame to file
				    *******************************************************/
				    //s32Ret = SAMPLE_COMM_VENC_SaveStream(enPayLoadType[i], pFile[i], &stStream);
#if 1				    
				    if(i == 0)
				    {
				    	//printf("add ENUM_VS_REC\r\n");
						//s32Ret = SAMPLE_COMM_VENC_WritePool(ENUM_VS_REC,&stStream);
				    }
					else if(i == 1)
					{
						//printf("add ENUM_VS_LIVE\r\n");
						//s32Ret = SAMPLE_COMM_VENC_WritePool(ENUM_VS_LIVE,&stStream);
					}
#else
					if(i == 0)
					{
						s32Ret = SAMPLE_COMM_VENC_WritePool(ENUM_VS_LIVE,&stStream);
					}
#endif
				    if (HI_SUCCESS != s32Ret)
				    {
				        free(stStream.pstPack);
				        stStream.pstPack = NULL;
				        SAMPLE_PRT("save stream failed!\n");
				        break;
				    }


					RgnHandle = 4 + i;
					OSD_Overlay_RGN_Display_Time(RgnHandle, i); 
				
				    /*******************************************************
				     step 2.5 : release stream
				    *******************************************************/
				    s32Ret = HI_MPI_VENC_ReleaseStream(i, &stStream);
				    if (HI_SUCCESS != s32Ret)
				    {
				        free(stStream.pstPack);
				        stStream.pstPack = NULL;
				        break;
				    }
				    /*******************************************************
				     step 2.6 : free pack nodes
				    *******************************************************/
				    free(stStream.pstPack);
				    stStream.pstPack = NULL;
				}
            }
        }

		usleep(1);
    }

    /*******************************************************
    * step 3 : close save-file
    *******************************************************/
#if WRITE_H264_FILE    
    for (i = 0; i < s32ChnTotal; i++)
    {
        fclose(pFile[i]);
    }
#endif

    return NULL;
}
#endif

int cdr_get_vencstream_fd(int s32venchn)
{
    //int i = 0;
    //HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s_vencFd = HI_FAILURE; //venc fd.
    //VENC_CHN_ATTR_S stVencChnAttr;

    /*********************************************
     step 1:  check & prepare save-file & venc-fd
    **********************************************/
    if ((s32venchn < 0) || (s32venchn >= VENC_MAX_CHN_NUM))
    {
        SAMPLE_PRT("Venc Channel is out of range !\n");
        return HI_FAILURE;
    }
    /* Get Venc Fd. */
    s_vencFd = HI_MPI_VENC_GetFd( s32venchn );
    if ( s_vencFd < 0 )
    {
        SAMPLE_PRT("HI_MPI_VENC_GetFd failed with %#x!\n", s_vencFd);
        return HI_FAILURE;
    }
	//printf("%s %d --s32venchn:%d,s_vencFd:%d\r\n",__FUNCTION__,__LINE__,s32venchn,s_vencFd);
	
    return s_vencFd;
}

static char g_recbuf[4194304] = {0}; //8M
static char g_livebuf[4194304] = {0}; //4M

HI_S32 write_stream_to_pool(int type, VENC_STREAM_S *pstStream,char *pTmpBuf)
{
	//pthread_mutex_lock( &g_MutexLock );
	HI_S32 i;
	//int iFrame = 0;    
	int iLen = 0;  //stream data size.

	sVAFrameInfo vaFrame;
	
	for (i = 0; i < pstStream->u32PackCount; i++)
	{
		iLen = 0;
		memcpy( pTmpBuf + iLen, pstStream->pstPack[i].pu8Addr+pstStream->pstPack[i].u32Offset, pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset);	
		iLen += pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset;

		vaFrame.u64PTS = pstStream->pstPack[i].u64PTS;		
		vaFrame.isIframe = pstStream->pstPack[i].DataType.enH264EType;
#if 0		
		if(iFrame == H264E_NALU_SPS || iFrame == H264E_NALU_PPS || iFrame == H264E_NALU_SEI)
			printf(":pstStream->u32PackCount:%d  i:%d   iFrame:%d ,iLen:%d\r\n",pstStream->u32PackCount,i,iFrame,iLen);
#endif		
		if(vaFrame.isIframe  == H264E_NALU_SEI)	continue;//这儿把SEI帧去掉.暂时没啥用途.
		
		StremPoolWrite( type, pTmpBuf, iLen, vaFrame);				
	}	
	//pthread_mutex_unlock( &g_MutexLock );
    return HI_SUCCESS;

}


int cdr_rec_video_thread(void)
{
	HI_S32 s32Ret = 0;
	int iLen = 0;  //stream data size.
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.
	//static int orgin_flag = 0;
	//int iFrame = 0;
	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	VENC_CHN_STAT_S stStat;

	s_vencFd  = cdr_get_vencstream_fd(gsrec_stPara.s32VencChn);
	s_maxFd   = s_vencFd + 1; //for select.
	s_vencChn = gsrec_stPara.s32VencChn; //current video encode channel.	
	//int i = 0;
	//int j = 0;
	//RGN_HANDLE RgnHandle = 0;

	//struct sched_param param;
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	

	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return 0;
	}
	sleep(1);
    unsigned int select_fail_count = 0;
	while( gsrec_stPara.bThreadStart)
	{
		//usleep(100);
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
                //wrap_sys_restart( );
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

#if 1
				//SAMPLE_COMM_VENC_WritePool(ENUM_VS_REC,&stStream);
				write_stream_to_pool(ENUM_VS_REC,&stStream,g_recbuf);
#endif

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
		}// end while.
	}
   
	if(pstPack)  
        free(pstPack);
	return 1;
}

//直播  将sensor 处的频视流数据  获取 并写入到pool 缓存池中。
int cdr_live_video_thread(void)
{
	HI_S32 s32Ret = 0;
	int iLen = 0;              //stream data size.
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.

	//int iFrame = 0;
	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	VENC_CHN_STAT_S stStat;

	s_vencFd  = cdr_get_vencstream_fd(gslive_stPara.s32VencChn);
	s_maxFd   = s_vencFd + 1;             //for select.
	s_vencChn = gslive_stPara.s32VencChn; //current video encode channel.	
	//int i = 0;
	//int j = 0;
	//RGN_HANDLE RgnHandle = 0;

	//struct sched_param param;
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	

	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return 0;
	}
	sleep(1);
	unsigned int select_fail_count = 0;
	while( gslive_stPara.bThreadStart)
	{
		
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			usleep(1000);
			select_fail_count ++; //modyfied: 2015-04-17.
			if (select_fail_count > 20)
			{
				printf("[%s, %d] ..........select failed count: %d ...........\n", __func__, __LINE__, select_fail_count);
				select_fail_count = 0;
			}else{
				continue;
			}
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

				write_stream_to_pool(ENUM_VS_LIVE,&stStream,g_livebuf);

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
		}// end while.
	}

	if(pstPack) 	free(pstPack);
	return 1;
}

#if(0)
int region_update_thread_pro(void)
{
	RGN_HANDLE RgnHandle = 0;
	
	while(1)
	{
		//printf("update region \r\n");
		RgnHandle =OSDCHN_COUNT + gsrec_stPara.s32VencChn;
		OSD_Overlay_RGN_Display_Time(RgnHandle, gsrec_stPara.s32VencChn); 

		RgnHandle =OSDCHN_COUNT + gslive_stPara.s32VencChn;
		OSD_Overlay_RGN_Display_Time(RgnHandle, gslive_stPara.s32VencChn); 


		RgnHandle =OSDCHN_COUNT + gsjpeg_stPara.s32VencChn;
		OSD_Overlay_RGN_Display_Time(RgnHandle, gsjpeg_stPara.s32VencChn); 
		sleep(1);		
	}
}
#else
int region_update_thread_pro(void)
{
	RGN_HANDLE RgnHandle = 0;
    
    unsigned char ucBuffTemp[60] = {0};
    unsigned int  ucBuffTempLen = 0;
    unsigned char ucBuffTemp2[60] = {0};
    unsigned int  ucBuffTempLen2 = 0;
    
	while(1)
	{
		//sleep(1);	
		usleep(500000);
		get_time_part_osd(ucBuffTemp,&ucBuffTempLen);
		get_osd_part_line2(ucBuffTemp2,&ucBuffTempLen2);

		RgnHandle = OSDCHN_COUNT + gsrec_stPara.s32VencChn;//录相通道
		OSD_Overlay_RGN_Display_Time(RgnHandle, gsrec_stPara.s32VencChn,ucBuffTemp,ucBuffTempLen,ucBuffTemp2); 

		RgnHandle =OSDCHN_COUNT + gslive_stPara.s32VencChn;//直播通道
		OSD_Overlay_RGN_Display_Time(RgnHandle, gslive_stPara.s32VencChn,ucBuffTemp,ucBuffTempLen,ucBuffTemp2); 

		RgnHandle =OSDCHN_COUNT + gsjpeg_stPara.s32VencChn;
		OSD_Overlay_RGN_Display_Time(RgnHandle, gsjpeg_stPara.s32VencChn,ucBuffTemp,ucBuffTempLen,ucBuffTemp2); 

	}
}

#endif

HI_S32 start_region_update_thread()
{
    pthread_t tfid;
    int ret = 0;
    ret = pthread_create(&tfid, NULL, (void *)region_update_thread_pro, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);
	return 0;
}



HI_S32 start_get_rec_video_stream()
{
    pthread_t tfid;
    int ret = 0;

	gsrec_stPara.bThreadStart = HI_TRUE;
	gsrec_stPara.s32VencChn = 0;
	gsrec_stPara.framerate = 30;
    ret = pthread_create(&tfid, NULL, (void *)cdr_rec_video_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);
	return 0;
}

//start 获取 直播的 视频数据流 任务
HI_S32 start_get_live_video_stream()
{
    pthread_t tfid;
    int ret = 0;
    gslive_stPara.bThreadStart = HI_TRUE;
    gslive_stPara.s32VencChn = 1;
    gslive_stPara.framerate = 30;

    ret = pthread_create(&tfid, NULL, (void *)cdr_live_video_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    pthread_detach(tfid);
    return 0;
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
