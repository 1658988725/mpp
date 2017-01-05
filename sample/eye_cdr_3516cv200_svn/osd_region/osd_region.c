#include <sys/ioctl.h>
#include "osd_region.h"
//#include "hkipc_hk.h"
#include "font.h"
#include "loadbmp.h"
#include "cdr_gps_data_analyze.h"

#include "cdr_config.h"

#define FONT_DIR  	"/usr/lib/ziku"
unsigned char *g_pASC48FontLib = NULL;
unsigned char *g_pASC32FontLib = NULL;
unsigned char *g_pASC24FontLib = NULL;
unsigned char *g_pASC16FontLib = NULL;

HI_S32 gs_s32RgnCnt = 2; //姣忎釜绐楀彛鍙犲姞鍖哄煙鏁帮細2


#define CHN_REC 0
#define CHN_LIVE 1
#define CHN_JPG 2
#define CHN_INDEX 3
#define CHN_MAX 4

#define BMP_RGN_WIDTH_REC  960+60 //1080 //720 //640 

/*字体16x32  8x16*/
#define FONT_TIME_1080_LAY_WIDTH   BMP_RGN_WIDTH_REC//640
#define FONT_TIME_1080_LAY_HEIGHT 32+32+2 
#define FONT_TIME_480_LAY_WIDTH 480
#define FONT_TIME_480_LAY_HEIGHT 20+20+2

#define LOGO_1080_LAY_WIDTH 220
#define LOGO_1080_LAY_HEIGHT 96
#define LOGO_480_LAY_WIDTH 110
#define LOGO_480_LAY_HEIGHT 48

/*画布坐标位置*/
#define FONT_TIME_1080_LAY_X  264
#define FONT_TIME_1080_LAY_Y  1014-32+4
#define FONT_TIME_480_LAY_X  122 //+100  字符的横向坐标
#define FONT_TIME_480_LAY_Y  446-16 //+100 字符的纵向坐标 -20为显示两行

#define LOGO_1080_LAY_X  30
#define LOGO_1080_LAY_Y  964
#define LOGO_480_LAY_X  12
#define LOGO_480_LAY_Y  426

static char OsdTimeFlag = 1;
static char OsdLactFlag = 1;

static char OsdPesonFlag = 1;
static char OsdSpeedFlag = 1;
static char OsdEnginFlag = 1;

static unsigned char ucOSOLogoSetFlag = 0x00;

HI_S32 FONT_ReadFile(const char *strFileName,unsigned char **ppFileBuf)
{
	char	szPath[60];
	int fileLen = 0;

	FILE *hFile = NULL;
	unsigned char *pBuffer=NULL;
	int nReadLen = 0;

	memset(szPath , 0 , sizeof(szPath));
	sprintf(szPath , "%s/%s" , FONT_DIR , strFileName);
	if(NULL == (hFile = fopen(szPath,"rb")))
	{
		return HI_FAILURE;
	}
	if(fseek(hFile,0,SEEK_END)!=0)
	{
		goto ERROR;
	}	

	fileLen = ftell(hFile);
	if(fseek(hFile,0,SEEK_SET)!=0)
	{
		goto ERROR;
	}
	if(NULL == (pBuffer = (unsigned char *)malloc(fileLen)))
	{
		goto ERROR;
	}
	memset(pBuffer,0,fileLen);
	nReadLen = fread(pBuffer,1,fileLen,hFile);
	if(nReadLen != fileLen)
	{
		goto ERROR;
	}	
	*ppFileBuf = pBuffer;
	if(NULL != hFile)
	{
		fclose(hFile);
		hFile = NULL;
	}	
	return fileLen;

ERROR:
	if(pBuffer)free(pBuffer);
	if(hFile)  fclose(hFile);
	return HI_FAILURE;	
}

HI_S32 FONT_Init()
{	
    /*
	if(HI_FAILURE == FONT_ReadFile("ASC48",&g_pASC48FontLib))
	{
		return HI_FAILURE;
	}
     */
    if(HI_FAILURE == FONT_ReadFile("ASC32",&g_pASC32FontLib))
	{
		return HI_FAILURE;
	}
    /*
    	if(HI_FAILURE == FONT_ReadFile("ASC24",&g_pASC24FontLib))
	{
		return HI_FAILURE;
	}
    */
    if(HI_FAILURE == FONT_ReadFile("ASC16",&g_pASC16FontLib))
	{
		return HI_FAILURE;
	}


	return HI_SUCCESS;
}


int Get_Current_DayTime(unsigned char *pTime)
{
	if (NULL == pTime)
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}
	memset(pTime, '\0', sizeof(pTime));

	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[32] = {0};

	time(&curTime);
	ptm = gmtime(&curTime);
#if DEV_NASC
	sprintf(tmp, "%04d/%02d/%02d %02d:%02d:%02d ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
#else
	sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
#endif
	tmp[strlen(tmp)] = '\0';
	//printf("[%s %d] tmp: %s\n", __func__, __LINE__, tmp);   
	strcpy(pTime, tmp);
	//printf("[%s %d] pTime: %s \n", __func__, __LINE__, pTime);   

	return 0;
}

/***************************************************************************
 * function: change region diplay position or change venc channel.
 * params:
 *      RgnHandle   :  overlay region handle.
 *      RgnVencChn  :  venc channel.
 *      ContentLen  :  overlay region display contents byte size.
 * note:
 *      mode HI_ID_GROUP only support channel 0.
 * return:
 *      0 on success, and -1 on error.
 ***************************************************************************/
int OSD_Overlay_RGN_Display_Change(RGN_HANDLE RgnHandle, 
  VENC_GRP RgnVencChn)
{
	HI_S32 s32Ret = HI_FAILURE;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	//RGN_ATTR_S stRgnAttr;

	stChn.enModId  = HI_ID_VENC;
	stChn.s32DevId = 0;//0;
	stChn.s32ChnId = RgnVencChn;//RgnVencChn; //change venc channel ?????

	s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}
  
	switch(RgnVencChn)
	{
		case CHN_REC://1080p
		case CHN_JPG:
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = FONT_TIME_1080_LAY_X;//OSD_POSITION_X;
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = FONT_TIME_1080_LAY_Y;//HEIGHT_HD1080P-OSD_POSITION_Y-ASC48_FONT_HEIGHT;
			break;
		case CHN_LIVE://576 画布的起始位
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = FONT_TIME_480_LAY_X;//OSD_POSITION_X;
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = FONT_TIME_480_LAY_Y;//HEIGHT_VGA-OSD_POSITION_Y-ASC24_FONT_WIDTH;
			break;
		default:		break;
	}

	s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/*************************************************************************
 * function: create overlay region handle, and attach it to venc channel.
 * params:
 *      RgnHandle   :  overlay region handle.
 *      RgnVencChn  :  venc channel the region handle attached to.
 *      ContentLen  :  overlay region display contents byte size.
 * return:
 *      0 on success, and -1 on error.
 ************************************************************************/
int OSD_Overlay_RGN_Handle_Init( RGN_HANDLE RgnHandle, 
       VENC_GRP RgnVencChn,unsigned char ucSwCmd)
{
	HI_S32 s32Ret = HI_FAILURE;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stChn;
	VENC_GRP VencGrp;
	RGN_CHN_ATTR_S stChnAttr;

	/******************************************
	*  step 1: create overlay regions
	*****************************************/
	stRgnAttr.enType                            = OVERLAY_RGN; //region type.
	stRgnAttr.unAttr.stOverlay.enPixelFmt 		= PIXEL_FORMAT_RGB_1555; //format.
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG: 
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = FONT_TIME_1080_LAY_WIDTH;//480;//ASC32_FONT_WIDTH * ContentLen; //8*Len byte
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = FONT_TIME_1080_LAY_HEIGHT;//32;//ASC32_FONT_HEIGHT; //ASC32: 16*32 <==>w*h point per charactor display            
			break;
		case CHN_LIVE:
#if 0            
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = FONT_TIME_480_LAY_WIDTH;// 560;//FONT_TIME_480_LAY_WIDTH;//ASC24_FONT_WIDTH * ContentLen; //8*Len byte
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = FONT_TIME_480_LAY_HEIGHT;//28;//ASC24_FONT_HEIGHT; //ASC24: 16*24 <==>w*h point per charactor display
#else
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = FONT_TIME_480_LAY_WIDTH;// 560;//FONT_TIME_480_LAY_WIDTH;//ASC24_FONT_WIDTH * ContentLen; //8*Len byte
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = FONT_TIME_480_LAY_HEIGHT;//28;//ASC24_FONT_HEIGHT; //ASC24: 16*24 <==>w*h point per charactor display
#endif
			break;
		default:
			
			break;
	}
	stRgnAttr.unAttr.stOverlay.u32BgColor		= 0;

	s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	SAMPLE_PRT("create handle:%d success!\n", RgnHandle);

	/***********************************************************
	* step 2: attach created region handle to venc channel.
	**********************************************************/
	VencGrp = RgnVencChn;
	stChn.enModId  = HI_ID_VENC;
	stChn.s32DevId = 0;//0;
	stChn.s32ChnId = RgnVencChn;//VencGrp;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow 	= HI_TRUE;
	stChnAttr.enType 	= OVERLAY_RGN;
#if(0)
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = OSD_POSITION_X;//60;// OSD_POSITION_X; //
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = OSD_POSITION_Y; //y position.
#else
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = 0;//60;// OSD_POSITION_X; //
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = 0; //y position.
#endif

    #if(1)
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 0;// 120;//30;// 50;//48; //[0,128]
    #else
    	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 120 ;//0;// 120;//30;// 50;//48; //[0,128]
    #endif

    if((g_cdr_systemconfig.sCdrVideoRecord.mode == 0x01)&&(RgnVencChn!= CHN_LIVE))//背景黑色
    {
        stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 128 ;
    }

    //if(ucSwCmd == 0x01)
        stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 128;//128; 
    //if(ucSwCmd == 0x00)
        //stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 0; 
    
	stChnAttr.unChnAttr.stOverlayChn.u32Layer 	     = 1;//RgnHandle; //0;

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

	//osd 反色	
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = 0;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 127;

	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d to %d) failed with %#x!\n", RgnHandle, VencGrp, s32Ret);
		return HI_FAILURE;
	}
	//SAMPLE_PRT("attach handle:%d, to venc channel:%d success, position x: %d, y: %d !\n", RgnHandle, stChn.s32ChnId, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y);

	return HI_SUCCESS;
}

/*
黑边功能
RgnHandle : 第几个画布
RgnVencChn :在哪个编码通道
ucSwCmd: 0x01:显示，0x00 关闭
ucLaction:0x01:上 ，0x02:下 
*/
int OSD_Overlay_RGN_Handle_Init1( RGN_HANDLE RgnHandle, 
       VENC_GRP RgnVencChn,unsigned char ucSwCmd,unsigned char ucLaction)
{
	HI_S32 s32Ret = HI_FAILURE;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stChn;
	VENC_GRP VencGrp;
	RGN_CHN_ATTR_S stChnAttr;

	/******************************************
	*  step 1: create overlay regions
	*****************************************/
	stRgnAttr.enType                            = OVERLAY_RGN;           //region type.
	stRgnAttr.unAttr.stOverlay.enPixelFmt 		= PIXEL_FORMAT_RGB_1555; //format.
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG: 
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = 1920;
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = 140;
			break;
		case CHN_LIVE:
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = 640;//FONT_TIME_480_LAY_WIDTH;// 560;//FONT_TIME_480_LAY_WIDTH;//ASC24_FONT_WIDTH * ContentLen; //8*Len byte
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = FONT_TIME_480_LAY_HEIGHT;//28;//ASC24_FONT_HEIGHT; //ASC24: 16*24 <==>w*h point per charactor display
			break;
		default:
			
			break;
	}
	stRgnAttr.unAttr.stOverlay.u32BgColor		= 0;
    	//stRgnAttr.unAttr.stOverlay.u32BgColor		= 0xff;

	s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	/***********************************************************
	* step 2: attach created region handle to venc channel.
	**********************************************************/
	VencGrp = RgnVencChn;
	stChn.enModId  = HI_ID_VENC;
	stChn.s32DevId = 0;//0;
	stChn.s32ChnId = RgnVencChn;//VencGrp;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow 	= HI_TRUE;
	stChnAttr.enType 	= OVERLAY_RGN;
    
#if(0)
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = OSD_POSITION_X;//60;// OSD_POSITION_X; //
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = OSD_POSITION_Y; //y position.
#else
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = 0;//60;// OSD_POSITION_X; //横坐标
	if(ucLaction == 0x01)stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = 0; //y position.
	if(ucLaction == 0x02)stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = 1080-140; //下y position.
	//if(ucLaction == 0x02)stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = 480-140; //下y position.
#endif

    //close
	if(ucSwCmd == 0x00)stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 0;// 120;//30;// 50;//48; //[0,128]
    //open
    	if(ucSwCmd == 0x01)stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 128 ;//0;// 120;//30;// 50;//48; //[0,128]


    if(ucSwCmd == 0x01)stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 128;//128; //
    if(ucSwCmd == 0x00)stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 0;
	//stChnAttr.unChnAttr.stOverlayChn.u32Layer 	     = 0;//RgnHandle; //0;
	stChnAttr.unChnAttr.stOverlayChn.u32Layer 	     = 0;//RgnHandle; //0;

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

	//osd 反色	
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = 0;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 127;

	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d to %d) failed with %#x!\n", RgnHandle, VencGrp, s32Ret);
		return HI_FAILURE;
	}
	//SAMPLE_PRT("attach handle:%d, to venc channel:%d success, position x: %d, y: %d !\n", RgnHandle, stChn.s32ChnId, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y);

	return HI_SUCCESS;
}

/*************************************************************************
 * function: create overlay region handle, and attach it to venc channel.
 * params:
 *      RgnHandle   :  overlay region handle.
 *      RgnVencChn  :  venc channel the region handle attached to.
 *      ContentLen  :  overlay region display contents byte size.
 * ucOSDSw:0x01 open osd,0x00 close osd
 
 * return:
 *      0 on success, and -1 on error.
 ************************************************************************/
int OSD_Logo_OverlayRgn_Handle_Init( RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn, 
                                     unsigned int ContentLen ,unsigned char ucOSDSw)
{
	HI_S32 s32Ret = HI_FAILURE;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stChn;
	VENC_GRP VencGrp;
	RGN_CHN_ATTR_S stChnAttr;

	/******************************************
	*  step 1: create overlay regions
	*****************************************/
	stRgnAttr.enType                            = OVERLAY_RGN; //region type.
	stRgnAttr.unAttr.stOverlay.enPixelFmt 		= PIXEL_FORMAT_RGB_1555; //format.
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG:            
             stRgnAttr.unAttr.stOverlay.stSize.u32Width  = LOGO_1080_LAY_WIDTH;//180
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = LOGO_1080_LAY_HEIGHT;//144
			break;            
		case CHN_LIVE:
			stRgnAttr.unAttr.stOverlay.stSize.u32Width  = LOGO_480_LAY_WIDTH;
			stRgnAttr.unAttr.stOverlay.stSize.u32Height = LOGO_480_LAY_HEIGHT;
			break;
		default:	break;
	}
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0;

	s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	SAMPLE_PRT("create handle:%d success!\n", RgnHandle);

	/***********************************************************
	* step 2: attach created region handle to venc channel.
	**********************************************************/
	VencGrp = RgnVencChn;
	stChn.enModId  = HI_ID_VENC;
	stChn.s32DevId = 0;//0;
	stChn.s32ChnId = RgnVencChn;//VencGrp;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow 	= HI_TRUE;
	stChnAttr.enType 	= OVERLAY_RGN;
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG:
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = LOGO_1080_LAY_X;//964; //xjl
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = LOGO_1080_LAY_Y;
			break;
		case CHN_LIVE:
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X    = LOGO_480_LAY_X;//426; //xjl
			stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y    = LOGO_480_LAY_Y;            
			break;
		default:
			break;
	}
    //图片 
    #if(1)
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 0;// 120;//30;// 50;//48; //[0,128]
    #else
    	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 120;//30;// 50;//48; //[0,128]
    #endif
    if((g_cdr_systemconfig.sCdrVideoRecord.mode == 0x01) &&(RgnVencChn!= CHN_LIVE))
    {
        stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha      = 128;
    }
	//stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha      = 128;//128; //
	if(ucOSDSw == 0x00)   
        stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 0;//close osd
	if(ucOSDSw == 0x01) 
        stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;// open

    stChnAttr.unChnAttr.stOverlayChn.u32Layer 	     = 1;//RgnHandle; //0;

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;//ogr

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d to %d) failed with %#x!\n", RgnHandle, VencGrp, s32Ret);
		return HI_FAILURE;
	}
	//SAMPLE_PRT("attach handle:%d, to venc channel:%d success, position x: %d, y: %d !\n", RgnHandle, stChn.s32ChnId, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X, stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y);

	return HI_SUCCESS;
}

//pRgnContent:要示的字符第一行
//pRgnContent2:第二行
int OSD_Overlay_RGN_Display_ASC16( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent,
const unsigned char *pRgnContent2)
{
    HI_S32 s32Ret = HI_FAILURE;
    BITMAP_S stBitmap;
    BITMAP_S stBitmap2;
    BITMAP_S stBitmap3;
        
	if ((NULL == pRgnContent) || (NULL == pRgnContent2))
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}

	/* HI_MPI_RGN_SetBitMap */
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap.u32Width = 480;//480
	stBitmap.u32Height = 20;
	unsigned int unLen1 = 2 * stBitmap.u32Width * stBitmap.u32Height;
	stBitmap.pData = (unsigned char *) malloc(unLen1); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap.pData, 0, unLen1);
	OSD_Rgb1555_8x16(stBitmap.u32Width, stBitmap.u32Height, pRgnContent, stBitmap.pData);

	/* HI_MPI_RGN_SetBitMap */
	stBitmap2.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap2.u32Width = 480;//480
	stBitmap2.u32Height = 20;
	unsigned int unLen2 = 2 * stBitmap2.u32Width * stBitmap2.u32Height;
	stBitmap2.pData = (unsigned char *) malloc(unLen2); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap2.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap2.pData, 0, unLen2);
	OSD_Rgb1555_8x16(stBitmap2.u32Width, stBitmap2.u32Height, pRgnContent2, stBitmap2.pData);

	/* HI_MPI_RGN_SetBitMap */
	stBitmap3.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap3.u32Width = 480;//480
	stBitmap3.u32Height = 20*2;
	unsigned int unLen3 = 2 * stBitmap3.u32Width * stBitmap3.u32Height *2;
	stBitmap3.pData = (unsigned char *) malloc(unLen3); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap3.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
    memset(stBitmap3.pData, 0, unLen3);
    memcpy(stBitmap3.pData,stBitmap.pData,unLen1);
    memcpy(stBitmap3.pData+unLen1,stBitmap2.pData,unLen2);

	//s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
    	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap3);

    if (NULL != stBitmap.pData)
	{
		free(stBitmap.pData);
		stBitmap.pData = NULL;
	}
    if (NULL != stBitmap2.pData)
	{
		free(stBitmap2.pData);
		stBitmap2.pData = NULL;
	}
    if (NULL != stBitmap3.pData)
	{
		free(stBitmap3.pData);
		stBitmap3.pData = NULL;
	}
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return 0;
}


int OSD_Overlay_RGN_Display_ASC24( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent,
const unsigned char *pRgnContent2)
{
    HI_S32 s32Ret = HI_FAILURE;
    BITMAP_S stBitmap;
    BITMAP_S stBitmap2;
    BITMAP_S stBitmap3;

	if ((NULL == pRgnContent) || (NULL == pRgnContent2))
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}
	
	/* HI_MPI_RGN_SetBitMap */
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap.u32Width = BMP_RGN_WIDTH_REC;//480;
	stBitmap.u32Height = 28;
	unsigned int unLen1 = 2 * stBitmap.u32Width * stBitmap.u32Height;
	stBitmap.pData = (unsigned char *) malloc(unLen1); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap.pData, 0, unLen1);
	OSD_Rgb1555_16x24(stBitmap.u32Width, stBitmap.u32Height, pRgnContent, stBitmap.pData);

    /* HI_MPI_RGN_SetBitMap */
	stBitmap2.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap2.u32Width = BMP_RGN_WIDTH_REC;
	stBitmap2.u32Height = 28;
	unsigned int unLen2 = 2 * stBitmap2.u32Width * stBitmap2.u32Height;
	stBitmap2.pData = (unsigned char *) malloc(unLen2); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap2.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap2.pData, 0, unLen2);
	OSD_Rgb1555_16x24(stBitmap2.u32Width, stBitmap2.u32Height, pRgnContent2, stBitmap2.pData);


    /* HI_MPI_RGN_SetBitMap */
	stBitmap3.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap3.u32Width = BMP_RGN_WIDTH_REC;
	stBitmap3.u32Height = 28*2;
	unsigned int unLen3 = 2 * stBitmap3.u32Width * stBitmap3.u32Height;
	stBitmap3.pData = (unsigned char *) malloc(unLen3); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap3.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap3.pData, 0, unLen3);
    memcpy(stBitmap3.pData,stBitmap.pData,unLen1);
    memcpy(stBitmap3.pData+unLen1,stBitmap2.pData,unLen2);

	//s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
    	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap3);


	if (NULL != stBitmap.pData)
	{
		free(stBitmap.pData);
		stBitmap.pData = NULL;
	}
    if (NULL != stBitmap2.pData)
	{
		free(stBitmap2.pData);
		stBitmap2.pData = NULL;
	}
    if (NULL != stBitmap3.pData)
	{
		free(stBitmap3.pData);
		stBitmap3.pData = NULL;
	}

	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return 0;
}
 

int OSD_Overlay_RGN_Display_ASC32( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent,
const unsigned char *pRgnContent2)
{
	HI_S32 s32Ret = HI_FAILURE;
	BITMAP_S stBitmap;
    BITMAP_S stBitmap2;
    BITMAP_S stBitmap3;

	if ((NULL == pRgnContent)||(NULL == pRgnContent2))
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}

	/* HI_MPI_RGN_SetBitMap */	
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap.u32Width = BMP_RGN_WIDTH_REC;// 480;
	stBitmap.u32Height = 36;
	unsigned int unLen1 = 2 * stBitmap.u32Width * stBitmap.u32Height;
	stBitmap.pData = (unsigned char *) malloc(unLen1); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap.pData, 0, unLen1);
	OSD_Rgb1555_16x32(stBitmap.u32Width, stBitmap.u32Height, pRgnContent, stBitmap.pData);
    
	/* HI_MPI_RGN_SetBitMap */	
	stBitmap2.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap2.u32Width = BMP_RGN_WIDTH_REC;
	stBitmap2.u32Height = 36;
	unsigned int unLen2 = 2 * stBitmap2.u32Width * stBitmap2.u32Height;
    stBitmap2.pData = (unsigned char *) malloc(unLen2); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap2.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap2.pData, 0, unLen2);
	OSD_Rgb1555_16x32(stBitmap2.u32Width, stBitmap2.u32Height, pRgnContent2, stBitmap2.pData);

    stBitmap3.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap3.u32Width = BMP_RGN_WIDTH_REC;
	stBitmap3.u32Height = 36*2;
	unsigned int unLen3 = 2 * stBitmap3.u32Width * stBitmap3.u32Height;
	stBitmap3.pData = (unsigned char *) malloc(unLen3); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap3.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}
	memset(stBitmap3.pData, 0, unLen3);
    memcpy(stBitmap3.pData,stBitmap.pData,unLen1);
    memcpy(stBitmap3.pData+unLen1,stBitmap2.pData,unLen2);
    
	//s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
    s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap3);

	if (NULL != stBitmap.pData)
	{
		free(stBitmap.pData);
		stBitmap.pData = NULL;
	}
    if (NULL != stBitmap2.pData)
	{
		free(stBitmap2.pData);
		stBitmap2.pData = NULL;
	}
    if (NULL != stBitmap3.pData)
	{
		free(stBitmap3.pData);
		stBitmap3.pData = NULL;
	}

	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return 0;
}


int OSD_Overlay_RGN_Display_ASC48( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent )
{
	HI_S32 s32Ret = HI_FAILURE;
	BITMAP_S stBitmap;


	if (NULL == pRgnContent)
	{
		printf("[%s, %d] error, NULL pointer transfered.\n", __FUNCTION__, __LINE__); 
		return -1;
	}

	//SAMPLE_PRT("RgnHandle: %d, pRgnContent: %s, ContentLen: %d,.\n", RgnHandle, pRgnContent, ContentLen);
#if 1
	/* HI_MPI_RGN_SetBitMap */	
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap.u32Width = 640;//480;
	stBitmap.u32Height = 50;//48;
	unsigned int unLen = 2 * stBitmap.u32Width * stBitmap.u32Height;

	stBitmap.pData = (unsigned char *) malloc(unLen); //RGB1555: 2 bytes(R:5 G:5 B:5).
	if (NULL == stBitmap.pData)
	{
		SAMPLE_PRT("malloc error with: %d, %s\n", errno, strerror(errno));  
		return HI_FAILURE;
	}

	memset(stBitmap.pData, 0, unLen);
	OSD_Rgb1555_24x48(stBitmap.u32Width, stBitmap.u32Height, pRgnContent, stBitmap.pData);
#else //usRgb1555_48x24
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	stBitmap.u32Width = 480;
	stBitmap.u32Height = 48;
	unsigned int unLen = 2 * stBitmap.u32Width * stBitmap.u32Height;
	stBitmap.pData =  (HI_U8 *)MemMalloc(unLen);

	Memset16(usRgb1555_24x48, OSD_BACKGROUNGHT, 34560);
	OSD_Rgb1555_24x48(stBitmap.u32Width, stBitmap.u32Height, pRgnContent, usRgb1555_24x48);
	memcpy((HI_U8 *)stBitmap.pData, usRgb1555_24x48, unLen);
#endif

#if 0
	printf("******************1****************\r\n");
	int i=0;
	//char *p = stBitmap.pData
	char p[10000];
	memset(p,0x00,sizeof(p));
	//memcpy(p,stBitmap.pData,stBitmap.u32Height*stBitmap.u32Width);
	for(i=0;i<unLen;i++)
	{
		printf("%02x ",p[i]);
		if(i%32 == 0)
			printf("\r\n");	}

	printf("******************2****************\r\n");
#endif	

	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
	if (NULL != stBitmap.pData)
	{
		free(stBitmap.pData);
		stBitmap.pData = NULL;
	}

	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return 0;
}


/******************************************************************************
* funciton : load bmp from file
******************************************************************************/
HI_S32 SAMPLE_RGN_LoadBmp(const HI_CHAR *filename, BITMAP_S *pstBitmap)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
        SAMPLE_PRT("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    pstBitmap->pData = malloc(2*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));

    if(NULL == pstBitmap->pData)
    {
        SAMPLE_PRT("malloc osd memroy err!\n");
        return HI_FAILURE;
    }

    CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pstBitmap->pData));

    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;
    pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_1555;
    return HI_SUCCESS;
}

int OSD_Overlay_RGN_Display_Logo( RGN_HANDLE RgnHandle,VENC_GRP RgnVencChn)
{
	HI_S32 s32Ret = HI_FAILURE;
	BITMAP_S stBitmap;
    
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG:
			s32Ret = SAMPLE_RGN_LoadBmp("/usr/ico/logo_219_96.bmp", &stBitmap);
			break;
		case CHN_LIVE:
             s32Ret = SAMPLE_RGN_LoadBmp("/usr/ico/logo_110_48.bmp", &stBitmap);
			break;
		default:
              s32Ret = SAMPLE_RGN_LoadBmp("/usr/ico/logo_110_48.bmp", &stBitmap);
			break;
	}

	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("load bmp failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

    int i, j;
    HI_U16* pu16Temp;
    pu16Temp = (HI_U16*)stBitmap.pData;
    for (i = 0; i < stBitmap.u32Height; i++)
    {
        for (j = 0; j < stBitmap.u32Width; j++)
        {
            if (0x8000 == *pu16Temp)
            {
                *pu16Temp = 0x0000;
            }
            pu16Temp++;
        }
    }
 
    
	s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
	if (NULL != stBitmap.pData)
	{
		free(stBitmap.pData);
		stBitmap.pData = NULL;
	}

	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	return 0;
}

 
int OSD_Overlay_RGN_Display_Time( RGN_HANDLE RgnHandle,VENC_GRP RgnVencChn,
unsigned char *buf_Time,unsigned int bufLen,unsigned char *ucpBuffLine2)
{
	OSD_Overlay_RGN_Display_Change( RgnHandle,RgnVencChn);
	switch(RgnVencChn)
	{
		case CHN_REC:
		case CHN_JPG:
			OSD_Overlay_RGN_Display_ASC32( RgnHandle,ucpBuffLine2,buf_Time);
             //OSD_Overlay_RGN_Display_ASC24( RgnHandle,ucpBuffLine2,buf_Time);
			break;
		case CHN_LIVE:
			OSD_Overlay_RGN_Display_ASC16( RgnHandle,ucpBuffLine2,buf_Time);
			break;
		default:	break;
	}

	return 0;
}


/******************************************************************************
* funciton : osd region show or hide
******************************************************************************/
HI_S32 OSD_ShowOrHide(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, HI_BOOL bShow)
{
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	HI_S32 s32Ret;

	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = VencGrp;
	stChn.s32ChnId = 0;

	s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", RgnHandle, s32Ret);
		return HI_FAILURE;
	}

	stChnAttr.bShow = bShow;
	s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n",\
			   RgnHandle, s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

#if(0)
int get_time_part_osd(unsigned char *pucBuff,int *pBufLen)
{	
    unsigned char buf_Time[32] = {0};
    unsigned char buf_laction[32] = {0};
    
	Get_Current_DayTime(buf_Time);
    sprintf(buf_laction,"N,%lf;E,%lf",g_GpsInfo.longitude,g_GpsInfo.latitude);

    sprintf(pucBuff,"%s;%s",buf_Time,buf_laction);

    *pBufLen = strlen(pucBuff);
    
    return 0;   
}
#else



int get_time_part_osd(unsigned char *pucBuff,int *pBufLen)
{	
    unsigned char buf_Time[32] = {0};
    unsigned char buf_laction[32] = {0};

    OsdTimeFlag = g_cdr_systemconfig.sCdrOsdCfg.time;
    OsdLactFlag = g_cdr_systemconfig.sCdrOsdCfg.position;

    OsdPesonFlag = g_cdr_systemconfig.sCdrOsdCfg.personalizedSignature;
    OsdSpeedFlag = g_cdr_systemconfig.sCdrOsdCfg.speed;
    OsdEnginFlag = g_cdr_systemconfig.sCdrOsdCfg.engineSpeed;
    
	Get_Current_DayTime(buf_Time);
    sprintf(buf_laction,"N%lf E%lf",g_GpsInfo.longitude,g_GpsInfo.latitude);

    if(OsdTimeFlag == 0x00) memset(buf_Time,'\0',sizeof(buf_Time));
    if(OsdLactFlag == 0x00) memset(buf_laction,'\0',sizeof(buf_laction));
    
    sprintf(pucBuff,"%s%s",buf_Time,buf_laction);

    *pBufLen = strlen(pucBuff);
    
    return 0;   
}
#endif

int get_EngineSpeed()
{
  int iEngineSpeed = 0;
  return iEngineSpeed;
}

int get_speed()
{
  int iSpeed = 0;
  return iSpeed;
}
unsigned char g_ucBuff[12] = "EYE KAKA ";
int get_PersonalizedSignature(unsigned char *pBuff)
{
  //10个字节
 memcpy(pBuff,g_ucBuff,sizeof(g_ucBuff));

 return 0;
}


// 个性签名，速度， 转速
int get_osd_part_line2(unsigned char *pucBuff,int *pBufLen)
{	    
    unsigned char buf_PersSign[32] = {0};
    unsigned char buf_Speed[32] = {0};
    unsigned char buf_Engine[32] = {0};

    //sprintf(buf_PersSign,"%s",get_PersonalizedSignature(ucBuff));    //EYE KAKA
    sprintf(buf_PersSign,"EYE KAKA ");    //EYE KAKA
    sprintf(buf_Speed,"Speed:%dKm/h ",get_speed());
    sprintf(buf_Engine,"Rpm:%dr/s",get_EngineSpeed());

    if(OsdSpeedFlag == 0x00) memset(buf_Speed,'\0',sizeof(buf_Speed));
    if(OsdEnginFlag == 0x00) memset(buf_Engine,'\0',sizeof(buf_Engine));
    if(OsdPesonFlag == 0x00) memset(buf_PersSign,'\0',sizeof(buf_PersSign));
    
    sprintf(pucBuff,"%s%s%s",buf_PersSign,buf_Speed,buf_Engine);

    *pBufLen = strlen(pucBuff);

    return 0;   
}



/*
ucIndex 0x01:enable all osd
ucIndex 0x02:enable log osd
ucCmd :0x00 close enable 0x01 open enable
*/
int cdr_osd_sw(unsigned char ucIndex,unsigned char ucCmd)
{
    VENC_GRP RgnVencChn = OSDCHN_COUNT; 
    int nChn = 0;
    RGN_HANDLE RgnHandle = 0;

    unsigned char buf_RGN[32] = {0};
    memset( buf_RGN, 'A', sizeof(buf_RGN)-1 );
    buf_RGN[31] = '\0';	
	unsigned int  DevBufLen = strlen(buf_RGN);

    unsigned char ucBuffTemp[60] = {0};
    unsigned int  ucBuffTempLen = 0;

    unsigned char ucBuffTemp2[60] = {0};
    unsigned int  ucBuffTempLen2 = 0;

    /*控制logo开与关*/
    if(g_cdr_systemconfig.sCdrOsdCfg.logo == 0x00){
        	for (nChn = 0; nChn < RgnVencChn; nChn++)
        	{
        		RgnHandle = nChn;
              HI_MPI_RGN_Destroy(RgnHandle);              
              ucOSOLogoSetFlag = 0x00;
        	}
    }else if(g_cdr_systemconfig.sCdrOsdCfg.logo == 0x01){
        
       if(ucOSOLogoSetFlag == 0x00){
         for (nChn = 0; nChn < RgnVencChn; nChn++){            
             RgnHandle = nChn;
		    OSD_Logo_OverlayRgn_Handle_Init(RgnHandle, nChn,DevBufLen,ucCmd); 
		    OSD_Overlay_RGN_Display_Logo(RgnHandle,nChn);      
         }
         ucOSOLogoSetFlag = 0x01;
       }
    }else{
        	for (nChn = 0; nChn < RgnVencChn; nChn++)
        	{
            RgnHandle = nChn + RgnVencChn;
            HI_MPI_RGN_Destroy(RgnHandle);

            get_time_part_osd(ucBuffTemp,&ucBuffTempLen);
            get_osd_part_line2(ucBuffTemp2,&ucBuffTempLen2);
            OSD_Overlay_RGN_Handle_Init( RgnHandle, nChn,ucCmd); 
            OSD_Overlay_RGN_Display_Time(RgnHandle,nChn,ucBuffTemp,ucBuffTempLen,ucBuffTemp2); //OSD_Overlay_RGN_Display_Time(RgnHandle,nChn); 

        	}
    }
}

/*************************************************************************
 * function:video osd init
 *other:wangshaoshu
 *************************************************************************/
int cdr_OsdInit()
{
	RGN_HANDLE RgnHandle = 0;
	VENC_GRP RgnVencChn = OSDCHN_COUNT;  // 1 main stream,1 slave stream,1 jgpeg stream
	int nChn = 0;

	if(HI_FAILURE== FONT_Init())
	{
		printf("FONT_Init FAILURE");
		return HI_FAILURE;	
	}

	unsigned char buf_RGN[32] = {0};
	memset( buf_RGN, '\0', sizeof(buf_RGN) );
	memset( buf_RGN, 'A', sizeof(buf_RGN)-1 );
	
	unsigned int  DevBufLen = strlen(buf_RGN);

    unsigned char ucBuffTemp[60] = {0};
    unsigned int  ucBuffTempLen = 0;
    unsigned char ucBuffTemp2[60] = {0};
    unsigned int  ucBuffTempLen2 = 0;


    OsdTimeFlag = g_cdr_systemconfig.sCdrOsdCfg.time;
    OsdLactFlag = g_cdr_systemconfig.sCdrOsdCfg.position;

    OsdPesonFlag = g_cdr_systemconfig.sCdrOsdCfg.personalizedSignature;
    OsdSpeedFlag = g_cdr_systemconfig.sCdrOsdCfg.speed;
    OsdEnginFlag = g_cdr_systemconfig.sCdrOsdCfg.engineSpeed;

    /*LOGO*/ 
	int iCmd ;//close logo
    iCmd = g_cdr_systemconfig.sCdrOsdCfg.logo;
    if(iCmd == 0x00) ucOSOLogoSetFlag = 0x00;
    if(iCmd == 0x01) ucOSOLogoSetFlag = 0x01;
    
	for (nChn = 0; nChn < RgnVencChn; nChn++)
	{
		RgnHandle = nChn;
		OSD_Logo_OverlayRgn_Handle_Init( RgnHandle, nChn,DevBufLen,iCmd); 
		OSD_Overlay_RGN_Display_Logo(RgnHandle,nChn); 
	}
    /*字幕*/
	RgnHandle = 0;
    //iCmd = g_cdr_systemconfig.sCdrOsdCfg.time;
    
    get_time_part_osd(ucBuffTemp,&ucBuffTempLen);
    get_osd_part_line2(ucBuffTemp2,&ucBuffTempLen2);
    
	for (nChn = 0; nChn < RgnVencChn; nChn++)
	{
        RgnHandle = RgnVencChn+nChn;
        OSD_Overlay_RGN_Handle_Init( RgnHandle, nChn,0x01);                
        OSD_Overlay_RGN_Display_Time(RgnHandle,nChn,ucBuffTemp,ucBuffTempLen,ucBuffTemp2);   
	}
    //1:live 通道
    if(g_cdr_systemconfig.sCdrVideoRecord.mode == 0x01){
       OSD_Overlay_RGN_Handle_Init1(6, 2,0x01,0x01);
       OSD_Overlay_RGN_Handle_Init1(7, 2,0x01,0x02);

       OSD_Overlay_RGN_Handle_Init1(8, 0,0x01,0x01);
       OSD_Overlay_RGN_Handle_Init1(9, 0,0x01,0x02);
    }
   
    return 0;
}




