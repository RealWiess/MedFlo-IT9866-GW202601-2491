#ifndef __RN6854_H__
#define __RN6854_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum RN68_TIMING_ID_TAG
{
    RN68_HD720P_25FPS = 0,
    RN68_HD720P_30FPS,
    RN68_FHD1080P_25FPS,
    RN68_FHD1080P_30FPS,
    RN68_D1NTSC_60FPS,
    RN68_D1PAL_50FPS,
    RN68_UNKNOWN,
} RN68_TIMING_ID;

typedef enum RN68_VIDEO_STATE_TAG
{
	RN_VIDEO_LOCKED = 0,
    RN_VIDEO_DECTECT,
    RN_VIDEO_UNKNOWN,
} RN68_VIDEO_STATE;

//X10LightDriver_t1.h
typedef struct RN6854SensorDriverStruct *RN6854SensorDriver;
SensorDriver RN6854SensorDriver_Create();
void RN6854SensorDriver_Destory(SensorDriver base);
void RN6854Initialize(uint16_t Mode);
void RN6854Terminate(void);
void RN6854OutputPinTriState(uint8_t flag);
uint16_t RN6854GetProperty(MODULE_GETPROPERTY property);
uint8_t RN6854GetStatus(MODULE_GETSTATUS Status);
void RN6854PowerDown(uint8_t enable);
uint8_t RN6854IsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif