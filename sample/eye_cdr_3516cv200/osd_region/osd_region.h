
#ifndef __OSD_REGION_H__
#define __OSD_REGION_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "sample_comm.h"

#define TEXT_DIRECTION  1	
#define TEXT_SLANT      0

#define MAX_OSD_FONTS		32

#define ASC_FONT_WIDTH      16//8  
#define ASC_FONT_HEIGHT     24//16 //ascii font standard:8*16 points for per charactor display

#define ASC24_FONT_WIDTH      16//8  
#define ASC24_FONT_HEIGHT     24//16 //ascii font standard:8*16 points for per charactor display

#define ASC32_FONT_WIDTH      16 
#define ASC32_FONT_HEIGHT     32//ascii font standard:16*32 points for per charactor display

#define ASC48_FONT_WIDTH      24 
#define ASC48_FONT_HEIGHT     48//ascii font standard:16*32 points for per charactor display


#define OSD_POSITION_X      16 //x ��ʼλ��
#define OSD_POSITION_Y      16 //y ��ʼλ��

#define WIDTH_HD720P    1280
#define WIDTH_VGA       640
#define WIDTH_QVGA      320
#define WIDTH_CIF       352

#define HEIGHT_VGA 480

#define HEIGHT_HD1080P 1080
#define WIDTH_HD1080P  1920


#define DEVID           "USER"
#define CONF_HKCLIENT   "/mnt/sif/hkclient.conf"

/*  here we list HTML 4.0 support color rgb value */
#define OSD_PALETTE_COLOR_BLACK		0x000000		// ��ɫ
#define OSD_PALETTE_COLOR_NAVY		0x000080		// ����ɫ
#define OSD_PALETTE_COLOR_GREEN		0x008000		// ��ɫ
#define OSD_PALETTE_COLOR_TEAL		0x008080		// ī��ɫ
#define OSD_PALETTE_COLOR_SILVER	0xC0C0C0		// ����ɫ
#define OSD_PALETTE_COLOR_BLUE		0x0000FF		// ��ɫ
#define OSD_PALETTE_COLOR_LIME		0x00FF00		// ǳ��ɫ
#define OSD_PALETTE_COLOR_AQUA		0x00FFFF		// ��ɫ
#define OSD_PALETTE_COLOR_MAROON	0x800000		// ��ɫ
#define OSD_PALETTE_COLOR_PURPLE	0x800080		// ��ɫ
#define OSD_PALETTE_COLOR_OLIVE		0x808000		// ���ɫ
#define OSD_PALETTE_COLOR_GRAY		0x808080		// ��ɫ
#define OSD_PALETTE_COLOR_RED		0xFF0000		// ��ɫ
#define OSD_PALETTE_COLOR_FUCHSIA	0xFF00FF		// Ʒ��ɫ
#define OSD_PALETTE_COLOR_YELLOW	0xFFFF00		// ��ɫ
#define OSD_PALETTE_COLOR_WHITE		0xFFFFFF		// ��ɫ

typedef struct __osd_region_st {
	int osd_rgn_enable;     //1:enable OSD region display,   0:disable.
	unsigned int pos_x;     //dispaly start position: x.
	unsigned int pos_y;     //dispaly start position: y.
	unsigned int width;		//region width
	unsigned int height;	//region height
	int osd_content_len;    //Indicate content length to display, 8 per byte.
	unsigned char osd_content_table[MAX_OSD_FONTS];   //OSD table, that is the contents to display.
	int win_palette_idx;    //window background color, GM8120 index from 0~15
} st_OSD_Region;

typedef struct st_Color
{
	unsigned int R; //red
	unsigned int G; //green
	unsigned int B; //blue
} Color;

typedef struct _CaptionAdd
{
	int mRowSpace;	// 2 �ֵ��м��
	int mColSpace;	// 2 �ֵ��м��
	int mStartRow;	// ���ֵ��ӵ���ʼ�У� Ĭ��Ϊͼ��߶�-20
	int mStartCol;	// ���ֵ��ӵ���ʼ�У� Ĭ��Ϊ10
	Color mTextColor; // �������ֵ���ɫ�� Ĭ��Ϊ��ɫ��
	unsigned char*mImagePtr;	// ͼ�����ʼ��ַ��
	int mImageWidth, mImageHeight;
	unsigned char *HZK16;	//����ģ�� buffer
	unsigned char *ASC16;	//ascii 
	unsigned char mat[16*2];	//��ģ	
}CaptionAdd;

#define OSDCHN_COUNT 3

HI_S32 OSD_ShowOrHide(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, HI_BOOL bShow);

int region_update_thread_pro(void);

int cdr_osd_sw(unsigned char ucIndex,unsigned char ucCmd);

//function call:
//int Get_Current_DayTime(unsigned char *pTime); //��ȡϵͳ��ǰ���ں�ʱ��
//int OSD_RGN_Init( st_OSD_Region *pOsdRgn, RGN_HANDLE RgnHandle, const char *pRgnContent ); //��ʼ��OSD��ʾ����
//int OSD_RGN_Display(st_OSD_Region *pOsdRgn, RGN_HANDLE RgnHandle, const char *pRgnContent, VENC_GRP RgnVencChn);//OSD������ʾ
//int OSD_RGN_Handle_Initialize(void); //׼��Ҫ��ʾ������,��ʼ��,������������
//int OSD_RGN_Handle_Display_Update(VENC_GRP RgnVencChn); //������ʾˢ��
//int OSD_RGN_Handle_Finalize(VENC_GRP RgnVencChn); //�����Ѿ�������������
//
//int OSD_Overlay_RGN_Handle_Init(RGN_HANDLE RgnHandle, unsigned int ContentLen, VENC_GRP RgnVencChn, int RgnVdWidth, int LorR);
//int OSD_Overlay_RGN_Display_Static( RGN_HANDLE RgnHandle, const unsigned char *pRgnContent );
//int OSD_Overlay_RGN_Handle_Finalize( RGN_HANDLE RgnHandle, VENC_GRP RgnVencChn );
#endif /* osd_region.h */

