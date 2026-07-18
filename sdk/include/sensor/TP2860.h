#ifndef __TP2860_H__
#define __TP2860_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TP2860_VIDEO_INPUT_STATUS_REG 0x01
#define TP2860_V_PLL_LOCK             0x40
#define TP2860_H_PLL_LOCK             0x20
#define TP2860_Carrier_PLL_LOCK       0x10
#define TP2860_Video_Dectect          0x08

typedef struct _TP2860RESOLUTIONINFO
{
    uint16_t width;
    uint16_t height;
    uint16_t framerate;
    uint16_t interlaced;
} TP2860RESOLUTIONINFO;

typedef enum TP2860_RESOLUTION_ID_TAG
{
    TP2860_HDA720P_25FPS = 0,
    TP2860_HDA720P_30FPS,
    TP2860_HDA1080P_25FPS,
    TP2860_HDA1080P_30FPS,
    TP2860_CVBS_NTSC,
    TP2860_CVBS_PAL,    
    TP2860_MAX_ID,
} TP2860_RESOLUTION_ID;

//X10LightDriver_t1.h
typedef struct TP2860SensorDriverStruct *TP2860SensorDriver;
SensorDriver TP2860SensorDriver_Create();
void TP2860SensorDriver_Destory(SensorDriver base);
void TP2860Initialize(uint16_t Mode);
void TP2860Terminate(void);
void TP2860OutputPinTriState(uint8_t flag);
uint16_t TP2860GetProperty(MODULE_GETPROPERTY property);
uint8_t TP2860GetStatus(MODULE_GETSTATUS Status);
void TP2860PowerDown(uint8_t enable);
uint8_t TP2860IsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif