#ifndef __TP2825B_H__
#define __TP2825B_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TP2825B_RES_TABLE_INDEX 9
#define BT1120_out  0
#define	en_960P     0
#define DetAutoStd  1
#define half_scaler 0
#define FLAG_LOSS       0x80
#define FLAG_LOCKED     0x60
#define FLAG_VIDEO_DECTECT     0x08
enum
{
    TP2802_1080P25 =	    0x03,
    TP2802_1080P30 =	    0x02,
    TP2802_720P25  =	    0x05,
    TP2802_720P30  =    	0x04,
    TP2802_720P50  =	    0x01,
    TP2802_720P60  =    	0x00,
    TP2802_SD      =        0x06,
    INVALID_FORMAT =		0x07,
    TP2802_720P25V2=	    0x0D,
    TP2802_720P30V2=		0x0C,
    TP2802_PAL	   =        0x0E,
    TP2802_NTSC	   =    	0x0F,
#if 0
    TP2802_3M18         =   0x20,   //2048x1536@18.75 for TVI
    TP2802_5M12         =   0x21,   //2592x1944@12.5 for TVI
//	TP2802_4M15         =   0x22,   //2688x1520@15 for TVI
    TP2802_3M20         =   0x23,   //2048x1536@20 for TVI
    TP2802_4M12         =   0x24,   //2688x1520@12.5 for TVI
    TP2802_6M10         =   0x25,   //3200x1800@10 for TVI
    TP2802_QHD30        =   0x26,   //2560x1440@30 for TVI/HDA/HDC
    TP2802_QHD25        =   0x27,   //2560x1440@25 for TVI/HDA/HDC
    TP2802_QHD15        =   0x28,   //2560x1440@15 for HDA
    TP2802_QXGA18       =   0x29,   //2048x1536@18 for HDA/TVI
    TP2802_QXGA30       =   0x2A,   //2048x1536@30 for HDA
    TP2802_QXGA25       =   0x2B,   //2048x1536@25 for HDA
    TP2802_4M30         =   0x2C,   //2688x1520@30 for TVI(for future)
    TP2802_4M25         =   0x2D,   //2688x1520@25 for TVI(for future)
    TP2802_5M20         =   0x2E,   //2592x1944@20 for TVI/HDA
    TP2802_8M15         =   0x2f,   //3840x2160@15 for TVI
    TP2802_8M12         =   0x30,   //3840x2160@12.5 for TVI
#endif
    TP2802_1080P15      =   0x31,   //1920x1080@15 for TVI
    TP2802_1080P60      =   0x32,   //1920x1080@60 for TVI
    TP2802_960P30       =   0x33,   //1280x960@30 for TVI
    TP2802_1080P20      =   0x34,    //1920x1080@20 for TVI
    TP2802_720P30HDR    =   0x35,    //1280x720@30 for TVI
    TP2802_WXGA			=   0x36,    //1280x800@30 for TVI
    TP2802_1080P27      =   0x37,   //1920x1080@27.5 for TVI
    TP2802_1080P24      =   0x38,   //1920x1080@24 for TVI
//	TP2802_1280x320P60  =   0x39,
    TP2802_960P25       =   0x3a,   //1280x960@25 for AHD
    TP2802_1080P50      =   0x3e,   //1920x1080@60 for TVI
    TP2802_720P25HDR    =   0x3f,    //1280x720@25 for TVI
};

enum
{
    VIDEO_UNPLUG,
    VIDEO_IN,
    VIDEO_LOCKED,
    VIDEO_UNLOCK
};

enum
{
    STD_TVI,
    STD_HDA,
    STD_HDC,
    STD_HDA_DEFAULT,
    STD_HDC_DEFAULT
};

enum
{
    TP2825B_VIN1 = 0,
    TP2825B_VIN2 = 1,
    TP2825B_VIN3,
    TP2825B_VIN4,
    TP2825B_VIN5,
    TP2825B_NA,
    TP2825B_Differential_VIN1P_VIN1N,
    TP2825B_Differential_VIN2P_VIN2N,
} TP2825B_INPUT_CTL;

enum
{
    PTZ_TVI      = 0,
    PTZ_TVI_720P = 1,
    PTZ_HDA_1    = 2,
    PTZ_HDA_2    = 3,
    PTZ_CVBS     = 4,
    PTZ_HDC_1    = 5,
    PTZ_960P     = 6,
    PTZ_960P30_AHD = 7,
};

typedef struct _TP2825B_RESOLUTIONINFO
{
    uint8_t  mode;
    uint16_t width;
    uint16_t height;
    uint16_t framerate;
    uint16_t interlaced;
} TP2825B_RESOLUTIONINFO;
//X10LightDriver_t1.h
typedef struct TP2825BSensorDriverStruct *TP2825BSensorDriver;
SensorDriver TP2825BSensorDriver_Create();
void TP2825BSensorDriver_Destory(SensorDriver base);
void TP2825BInitialize(uint16_t Mode);
void TP2825BTerminate(void);
void TP2825BOutputPinTriState(uint8_t flag);
uint16_t TP2825BGetProperty(MODULE_GETPROPERTY property);
uint8_t TP2825BGetStatus(MODULE_GETSTATUS Status);
void TP2825BPowerDown(uint8_t enable);
uint8_t TP2825BIsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif