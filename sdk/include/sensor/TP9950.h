#ifndef __TP9950_H__
#define __TP9950_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REG_VIDEO_INPUT_STATUS 0x01
#define REG_V_PLL_LOCK         0x40
#define REG_H_PLL_LOCK         0x20
#define REG_Carrier_PLL_LOCK   0x10
#define REG_Video_Dectect      0x08

typedef struct _RESOLUTIONINFO
{
    uint16_t width;
    uint16_t height;
    uint16_t framerate;
    uint16_t interlaced;
} RESOLUTIONINFO;

typedef enum TP9950_RESOLUTION_ID_TAG
{
    TP9950_CVBS_NTSC = 0,
    TP9950_CVBS_PAL,
    TP9950_HDA720P_25FPS,
    TP9950_HDA720P_30FPS,
    TP9950_HDA1080P_25FPS,
    TP9950_HDA1080P_30FPS,
    TP9950_TVI720P_30FPS,
    TP9950_TVI1080P_30FPS,
    TP9950_MAX_ID,
} TP9950_RESOLUTION_ID;

//X10LightDriver_t1.h
typedef struct TP9950SensorDriverStruct *TP9950SensorDriver;
SensorDriver TP9950SensorDriver_Create();
void TP9950SensorDriver_Destory(SensorDriver base);
void TP9950Initialize(uint16_t Mode);
void TP9950Terminate(void);
void TP9950OutputPinTriState(uint8_t flag);
uint16_t TP9950GetProperty(MODULE_GETPROPERTY property);
uint8_t TP9950GetStatus(MODULE_GETSTATUS Status);
void TP9950PowerDown(uint8_t enable);
uint8_t TP9950IsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif