#ifndef __RN6752_H__
#define __RN6752_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum RN67_TIMING_ID_TAG
{
    RN67_HD720P_25FPS = 0,
    RN67_HD720P_30FPS,
    RN67_FHD1080P_25FPS,
    RN67_FHD1080P_30FPS,
    RN67_D1NTSC_60FPS,
    RN67_D1PAL_50FPS,
    RN67_UNKNOWN,
} RN67_TIMING_ID;

typedef enum RN67_VIDEO_STATE_TAG
{
	RN67_VIDEO_LOCKED = 0,
    RN67_VIDEO_DECTECT,
    RN67_VIDEO_UNKNOWN,
} RN67_VIDEO_STATE;

//X10LightDriver_t1.h
typedef struct RN6752SensorDriverStruct *RN6752SensorDriver;
SensorDriver RN6752SensorDriver_Create();
void RN6752SensorDriver_Destory(SensorDriver base);
void RN6752Initialize(uint16_t Mode);
void RN6752Terminate(void);
void RN6752OutputPinTriState(uint8_t flag);
uint16_t RN6752GetProperty(MODULE_GETPROPERTY property);
uint8_t RN6752GetStatus(MODULE_GETSTATUS Status);
void RN6752PowerDown(uint8_t enable);
uint8_t RN6752IsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif