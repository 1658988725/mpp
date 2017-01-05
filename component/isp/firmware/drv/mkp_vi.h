/******************************************************************************

Copyright (C), 2004-2020, Hisilicon Tech. Co., Ltd.

******************************************************************************
File Name     : hi_vi.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2010/11/18
Last Modified :
Description   :
Function List :
History       :
1.Date        : 2010/11/18
  Author      : p123320/w54723/n168968
  Modification: Created file

******************************************************************************/

#ifndef  __MKP_VI_H__
#define  __MKP_VI_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "mkp_ioctl.h"
#include "hi_common.h"
#include "hi_comm_vi.h"

#ifdef MORPHO_DIS_ON
#include "hi_inner_vi.h"
#endif

/* high 8bit is port, middle 8bit is dev, low 8bit is chn */
#define VIU_MAKE_IOCFD(dev,way,chn)     ((((dev) & 0xff) << 16) | (((way) & 0xff) << 8) | ((chn) & 0xff))
#define VIU_GETDEV_BY_IOCFD(f)          (((f) >> 16) & 0xff)
#define VIU_GETWAY_BY_IOCFD(f)          (((f) >> 8) & 0xff)
#define VIU_GETCHN_BY_IOCFD(f)          ((f) & 0xff)

#define VI_TRACE(level, fmt...)\
    do{ \
        HI_TRACE(level, HI_ID_VIU, "[Func]:%s [Line]:%d [Info]:", __FUNCTION__, __LINE__);\
        HI_TRACE(level, HI_ID_VIU, ##fmt);\
    }while(0)
/*-------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////
typedef enum hiVI_FPN_WORK_MODE_E
{
    FPN_MODE_NONE	= 0x0,
    FPN_MODE_CORRECTION ,
    FPN_MODE_CALIBRATE,
    FPN_MODE_BUTT
} VI_FPN_WORK_MODE_E;

typedef enum hiVI_FPN_TYPE_E
{
    VI_FPN_TYPE_FRAME = 0,
    VI_FPN_TYPE_LINE = 1,
    VI_FPN_TYPE_BUTT
} VI_FPN_TYPE_E;

typedef enum hiVI_FPN_OP_TYPE_E
{
    VI_FPN_OP_TYPE_AUTO    = 0,
    VI_FPN_OP_TYPE_MANUAL  = 1,
    VI_FPN_OP_TYPE_BUTT
} VI_FPN_OP_TYPE_E;

typedef struct hiVI_FPN_FRAME_INFO_S
{
    HI_U32              u32Iso;             /* FPN CALIBRATE ISO */
    HI_U32              u32Offset;          /* FPN frame u32Offset (agv pixel value) */
    HI_U32              u32FrmSize;         /* FPN frame size (exactly frm size or compress len) */
    VIDEO_FRAME_INFO_S  stFpnFrame;         /* FPN frame info, 8bpp,10bpp,12bpp,16bpp. Compression or not */
} VI_FPN_FRAME_INFO_S;

typedef struct hiVI_USR_GET_FRM_TIMEOUT_S
{
    VIDEO_FRAME_INFO_S  stVFrame;
    HI_S32              s32MilliSec;
} VI_USR_GET_FRM_TIMEOUT_S;

typedef struct hiVI_USR_SEND_RAW_TIMEOUT_S
{
    VI_RAW_DATA_INFO_S  stRawDataInfo;
    HI_S32              s32MilliSec;
} VI_USR_SEND_RAW_TIMEOUT_S;


typedef struct hiVIU_FPN_CALIBRATE_ATTR_S
{
    HI_U32                  u32Threshold;        /* white_level,pix value > threshold means defective pixel */
    HI_U32                  u32FrameNum;         /* RW, value is 2^N, range: [0x1, 0x10] */
    VI_FPN_FRAME_INFO_S     stFpnCaliFrame;      /* RW, output in tune mode. */
} VI_FPN_CALIBRATE_ATTR_S;

typedef struct hiVIU_FPN_CORRECTION_ATTR_S
{
    HI_BOOL                 bEnable;
    HI_U32                  u32Gain;             /* RO, only use in manual mode, format 2.8 */
    VI_FPN_OP_TYPE_E        enFpnOpType;
    VI_FPN_FRAME_INFO_S     stFpnFrmInfo;        /* RO, input in correction mode. */
} VI_FPN_CORRECTION_ATTR_S;

typedef struct hiVI_FPN_ATTR_S
{
    VI_FPN_TYPE_E            enFpnType;
    VI_FPN_WORK_MODE_E       enFpnWorkMode;
    union
    {
        VI_FPN_CALIBRATE_ATTR_S     stCalibrate;
        VI_FPN_CORRECTION_ATTR_S    stCorrection;
    };
} VI_FPN_ATTR_S;

//typedef struct hiVI_DIS_ATTR_S
//{
//    HI_BOOL bEnable;
//} VI_DIS_ATTR_S;

typedef enum hiIOC_NR_VIU_E
{
    IOC_NR_VIU_DEV_ATTR_SET = 0,
    IOC_NR_VIU_DEV_EXATTR_SET,
    IOC_NR_VIU_DEV_ATTR_GET,
    IOC_NR_VIU_DEV_EXATTR_GET,

    IOC_NR_VIU_DEV_ENABLE,
    IOC_NR_VIU_DEV_DISABLE,

    IOC_NR_VIU_CHN_BIND,
    IOC_NR_VIU_CHN_GETBIND,
    IOC_NR_VIU_CHN_UNBIND,

    IOC_NR_VIU_CHN_ATTR_SET,
    IOC_NR_VIU_CHN_ATTR_GET,

    IOC_NR_VIU_CHN_MINORATTR_SET,
    IOC_NR_VIU_CHN_MINORATTR_GET,
    IOC_NR_VIU_CHN_MINORATTR_CLR,
    IOC_NR_VIU_CHN_ENABLE,
    IOC_NR_VIU_CHN_DISABLE,
    IOC_NR_VIU_CHN_INT_ENABLE,
    IOC_NR_VIU_CHN_INT_DISABLE,

    IOC_NR_VIU_FRAME_DEPTH_SET,
    IOC_NR_VIU_FRAME_DEPTH_GET,
    IOC_NR_VIU_FRAME_GET,
    IOC_NR_VIU_FRAME_RELEASE,

    IOC_NR_VIU_SET_USER_PIC,
    IOC_NR_VIU_ENABLE_USER_PIC,
    IOC_NR_VIU_DISABLE_USER_PIC,

    IOC_NR_VIU_CHN_QUERYSTAT,
    IOC_NR_VIU_BIND_FLAG2FD,

    IOC_NR_VIU_DELAY_TIME_SET_CTRL,

    IOC_NR_VIU_DEV_FLASH_CONFIG_SET,
    IOC_NR_VIU_DEV_FLASH_CONFIG_GET,
    IOC_NR_VIU_DEV_FLASH_CONFIG_TRIGGER,

    IOC_NR_VIU_DEV_CSC_ATTR_SET,
    IOC_NR_VIU_DEV_CSC_ATTR_GET,

    IOC_NR_VIU_CHN_LDC_ATTR_SET,
    IOC_NR_VIU_CHN_LDC_ATTR_GET,
    IOC_NR_VIU_CHN_FISHEYE_LDC_ATTR_SET,

    IOC_NR_VIU_CHN_ROTATE_SET,
    IOC_NR_VIU_CHN_ROTATE_GET,

    IOC_NR_VIU_CHN_LUMA_GET,

    IOC_NR_VIU_EXTCHN_ATTR_SET,
    IOC_NR_VIU_EXTCHN_ATTR_GET,

    IOC_NR_VIU_EXTCHN_CROP_SET,
    IOC_NR_VIU_EXTCHN_CROP_GET,

    IOC_NR_VIU_WDR_ATRR_SET,
    IOC_NR_VIU_WDR_ATRR_GET,

    IOC_NR_VIU_DEV_DUMP_ATTR_SET,
    IOC_NR_VIU_DEV_DUMP_ATTR_GET,

    IOC_NR_VIU_ENABLE_BAYER_DUMP,
    IOC_NR_VIU_DISABLE_BAYER_DUMP,

    IOC_NR_VIU_BIND_ISP,
    IOC_NR_VIU_UNBIND_ISP,

    IOC_NR_VIU_FPN_ATTR_SET,
    IOC_NR_VIU_FPN_ATTR_GET,

    IOC_NR_VIU_ENABLE_BAYER_READ,
    IOC_NR_VIU_DISABLE_BAYER_READ,
    IOC_NR_VIU_SEND_BAYER_DATA,

    IOC_NR_VIU_DIS_ATTR_SET,
    IOC_NR_VIU_DIS_ATTR_GET,

    IOC_NR_VIU_DCI_PARAM_SET,
    IOC_NR_VIU_DCI_PARAM_GET,

    IOC_NR_VIU_CHN_FISHEYE_CONFIG_SET,
    IOC_NR_VIU_CHN_FISHEYE_CONFIG_GET,

    IOC_NR_VIU_CHN_FISHEYE_ATTR_SET,
    IOC_NR_VIU_CHN_FISHEYE_ATTR_GET,

    IOC_NR_VIU_MOD_PARAM_SET,
    IOC_NR_VIU_MOD_PARAM_GET,
    IOC_NR_VIU_CHN_USRFRM_MODE_SET,

#ifdef MORPHO_DIS_ON
    IOC_NR_VIU_DIS_INIT_CTRL,
    IOC_NR_VIU_DIS_EXIT_CTRL,
    IOC_NR_VIU_DIS_CONFIG_SET,
    IOC_NR_VIU_DIS_CONFIG_GET,
    IOC_NR_VIU_DIS_INFO_GET,
    IOC_NR_VIU_DIS_INFO_SET,
    IOC_NR_VIU_DIS_DYNAMICATTR_SET_CTRL,
    IOC_NR_VIU_DIS_DYNAMICATTR_GET_CTRL,
    IOC_NR_VIU_DIS_DISABLE_CTRL,
    IOC_NR_VIU_PYRAMID_INFO_GET,
    IOC_NR_PYRAMID_STATE_GET,
    IOC_NR_VIU_DIS_FRM_DROP
#endif
} IOC_NR_VIU_E;

#define VIU_DEV_ATTR_SET_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_ATTR_SET, VI_DEV_ATTR_S)
#define VIU_DEV_EXATTR_SET_CTRL             _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_EXATTR_SET, VI_DEV_ATTR_EX_S)
#define VIU_DEV_ATTR_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DEV_ATTR_GET, VI_DEV_ATTR_S)
#define VIU_DEV_EXATTR_GET_CTRL             _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DEV_EXATTR_GET, VI_DEV_ATTR_EX_S)

#define VIU_DEV_ENABLE_CTRL                 _IO(IOC_TYPE_VIU, IOC_NR_VIU_DEV_ENABLE)
#define VIU_DEV_DISABLE_CTRL                _IO(IOC_TYPE_VIU, IOC_NR_VIU_DEV_DISABLE)

#define VIU_CHN_BIND_CTRL                   _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_BIND, VI_CHN_BIND_ATTR_S)
#define VIU_CHN_GETBIND_CTRL                _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_GETBIND, VI_CHN_BIND_ATTR_S)
#define VIU_CHN_UNBIND_CTRL                 _IO(IOC_TYPE_VIU, IOC_NR_VIU_CHN_UNBIND)

#define VIU_CHN_ATTR_SET_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_ATTR_SET, VI_CHN_ATTR_S)
#define VIU_CHN_ATTR_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_ATTR_GET, VI_CHN_ATTR_S)

#define VIU_CHN_ENABLE_CTRL                 _IO(IOC_TYPE_VIU, IOC_NR_VIU_CHN_ENABLE)
#define VIU_CHN_DISABLE_CTRL                _IO(IOC_TYPE_VIU, IOC_NR_VIU_CHN_DISABLE)
#define VIU_CHN_ENABLE_INT_CTRL             _IO(IOC_TYPE_VIU, IOC_NR_VIU_CHN_INT_ENABLE)
#define VIU_CHN_DISABLE_INT_CTRL            _IO(IOC_TYPE_VIU, IOC_NR_VIU_CHN_INT_DISABLE)

#define VIU_FRAME_DEPTH_SET_CTRL            _IOW(IOC_TYPE_VIU, IOC_NR_VIU_FRAME_DEPTH_SET, HI_U32)
#define VIU_FRAME_DEPTH_GET_CTRL            _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_FRAME_DEPTH_GET, HI_U32)
#define VIU_FRAME_GET_CTRL                  _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_FRAME_GET, VI_USR_GET_FRM_TIMEOUT_S)
#define VIU_FRAME_RELEASE_CTRL              _IOW(IOC_TYPE_VIU, IOC_NR_VIU_FRAME_RELEASE, VIDEO_FRAME_INFO_S)

#define VIU_SET_USER_PIC_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_SET_USER_PIC, VI_USERPIC_ATTR_S)
#define VIU_ENABLE_USER_PIC_CTRL            _IO(IOC_TYPE_VIU, IOC_NR_VIU_ENABLE_USER_PIC)
#define VIU_DISABLE_USER_PIC_CTRL           _IO(IOC_TYPE_VIU, IOC_NR_VIU_DISABLE_USER_PIC)

#define VIU_CHN_QUERYSTAT_CTRL              _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_QUERYSTAT, VI_CHN_STAT_S)
#define VIU_BIND_FLAG2FD                    _IOW(IOC_TYPE_VIU, IOC_NR_VIU_BIND_FLAG2FD, HI_U32)

#define VIU_DELAY_TIME_SET_CTRL             _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DELAY_TIME_SET_CTRL, HI_U32)

#define VIU_DEV_FLASH_SET_CTRL              _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_FLASH_CONFIG_SET, VI_FLASH_CONFIG_S)
#define VIU_DEV_FLASH_GET_CTRL              _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DEV_FLASH_CONFIG_GET, VI_FLASH_CONFIG_S)
#define VIU_DEV_FLASH_TRIGGER_CTRL          _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_FLASH_CONFIG_TRIGGER, HI_BOOL)

#define VIU_DEV_CSC_SET_CTRL                _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_CSC_ATTR_SET, VI_CSC_ATTR_S)
#define VIU_DEV_CSC_GET_CTRL                _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DEV_CSC_ATTR_GET, VI_CSC_ATTR_S)

#define VIU_CHN_LDC_SET_CTRL                _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_LDC_ATTR_SET, VI_LDC_ATTR_S)
#define VIU_CHN_LDC_GET_CTRL                _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_LDC_ATTR_GET, VI_LDC_ATTR_S)
#define VIU_CHN_FISHEYE_LDC_SET_CTRL        _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_FISHEYE_LDC_ATTR_SET, HI_U32) // the addr is passed implicitly

#define VIU_CHN_ROTATE_SET_CTRL             _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_ROTATE_SET, ROTATE_E)
#define VIU_CHN_ROTATE_GET_CTRL             _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_ROTATE_GET, ROTATE_E)

#define VIU_CHN_LUMA_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_LUMA_GET, VI_CHN_LUM_S)

#define VIU_EXTCHN_ATTR_SET_CTRL            _IOW(IOC_TYPE_VIU, IOC_NR_VIU_EXTCHN_ATTR_SET, VI_EXT_CHN_ATTR_S)
#define VIU_EXTCHN_ATTR_GET_CTRL            _IOR(IOC_TYPE_VIU, IOC_NR_VIU_EXTCHN_ATTR_GET, VI_EXT_CHN_ATTR_S)

#define VIU_EXTCHN_CROP_SET_CTRL            _IOW(IOC_TYPE_VIU, IOC_NR_VIU_EXTCHN_CROP_SET, CROP_INFO_S)
#define VIU_EXTCHN_CROP_GET_CTRL            _IOR(IOC_TYPE_VIU, IOC_NR_VIU_EXTCHN_CROP_GET, CROP_INFO_S)

#define VIU_WDR_ATTR_SET_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_WDR_ATRR_SET, VI_WDR_ATTR_S)
#define VIU_WDR_ATTR_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_WDR_ATRR_GET, VI_WDR_ATTR_S)

#define VIU_DEV_DUMP_ATTR_SET_CTRL          _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DEV_DUMP_ATTR_SET, VI_DUMP_ATTR_S)
#define VIU_DEV_DUMP_ATTR_GET_CTRL          _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DEV_DUMP_ATTR_GET, VI_DUMP_ATTR_S)

#define VIU_ENABLE_BAYER_DUMP_CTRL          _IO(IOC_TYPE_VIU, IOC_NR_VIU_ENABLE_BAYER_DUMP)
#define VIU_DISABLE_BAYER_DUMP_CTRL         _IO(IOC_TYPE_VIU, IOC_NR_VIU_DISABLE_BAYER_DUMP)

#define VIU_FPN_ATTR_SET_CTRL               _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_FPN_ATTR_SET, VI_FPN_ATTR_S)
#define VIU_FPN_ATTR_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_FPN_ATTR_GET, VI_FPN_ATTR_S)

#define VIU_ENABLE_BAYER_READ_CTRL          _IO(IOC_TYPE_VIU, IOC_NR_VIU_ENABLE_BAYER_READ)
#define VIU_DISABLE_BAYER_READ_CTRL         _IO(IOC_TYPE_VIU, IOC_NR_VIU_DISABLE_BAYER_READ)
#define VIU_SEND_BAYER_DATA                 _IOW(IOC_TYPE_VIU, IOC_NR_VIU_SEND_BAYER_DATA, VI_USR_SEND_RAW_TIMEOUT_S)

#define VIU_DIS_ATTR_SET_CTRL               _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_DIS_ATTR_SET, VI_DIS_ATTR_S)
#define VIU_DIS_ATTR_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DIS_ATTR_GET, VI_DIS_ATTR_S)

#define VIU_DCI_PARAM_SET_CTRL              _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_DCI_PARAM_SET, VI_DCI_PARAM_S)
#define VIU_DCI_PARAM_GET_CTRL              _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DCI_PARAM_GET, VI_DCI_PARAM_S)

#define VIU_CHN_FISHEYE_CONFIG_SET_CTRL     _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_FISHEYE_CONFIG_SET, FISHEYE_CONFIG_S)
#define VIU_CHN_FISHEYE_CONFIG_GET_CTRL     _IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_FISHEYE_CONFIG_GET, FISHEYE_CONFIG_S)

#define VIU_CHN_FISHEYE_ATTR_SET_CTRL     	_IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_FISHEYE_ATTR_SET, FISHEYE_DRV_ATTR_S)
#define VIU_CHN_FISHEYE_ATTR_GET_CTRL     	_IOR(IOC_TYPE_VIU, IOC_NR_VIU_CHN_FISHEYE_ATTR_GET, FISHEYE_ATTR_S)

#ifdef MORPHO_DIS_ON
#define VIU_PYRAMID_INFO_GET                _IOR(IOC_TYPE_VIU, IOC_NR_VIU_PYRAMID_INFO_GET, VI_DIS_PYRAMID_INFO_S)
#define VIU_DIS_INFO_GET                    _IOWR(IOC_TYPE_VIU, IOC_NR_VIU_DIS_INFO_GET, VI_DIS_INFO_S)
#define VIU_DIS_INFO_SET                    _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DIS_INFO_SET, VI_DIS_INFO_S)
#define VIU_DIS_CONFIG_SET                  _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DIS_CONFIG_SET, VI_DIS_CONFIG_S)
#define VIU_DIS_CONFIG_GET                  _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DIS_CONFIG_GET, VI_DIS_CONFIG_S)
#define VIU_DIS_INIT_CTRL                	_IOW(IOC_TYPE_VIU, IOC_NR_VIU_DIS_INIT_CTRL, VI_DIS_MMZ_S)
#define VIU_DIS_EXIT_CTRL                	_IO(IOC_TYPE_VIU, IOC_NR_VIU_DIS_EXIT_CTRL)
#define VIU_DIS_DYNAMICATTR_SET_CTRL        _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DIS_DYNAMICATTR_SET_CTRL, VI_DIS_ATTR_S)
#define VIU_DIS_DYNAMICATTR_GET_CTRL        _IOR(IOC_TYPE_VIU, IOC_NR_VIU_DIS_DYNAMICATTR_GET_CTRL, VI_DIS_ATTR_S)
#define VIU_DIS_FRM_DROP_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_DIS_FRM_DROP, VI_DIS_INFO_S)
#endif

#define VIU_MOD_PARAM_SET_CTRL               _IOW(IOC_TYPE_VIU, IOC_NR_VIU_MOD_PARAM_SET, VI_MOD_PARAM_S)
#define VIU_MOD_PARAM_GET_CTRL               _IOR(IOC_TYPE_VIU, IOC_NR_VIU_MOD_PARAM_GET, VI_MOD_PARAM_S)

#define VIU_CHN_USRFRM_MODE_SET_CTRL         _IOW(IOC_TYPE_VIU, IOC_NR_VIU_CHN_USRFRM_MODE_SET, VI_CHN_USRFRM_MODE_E)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __MPI_PRIV_VI_H__*/

