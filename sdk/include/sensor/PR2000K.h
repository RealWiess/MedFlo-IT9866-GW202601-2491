#ifndef __PR2000K_H__
#define __PR2000K_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PR2K_RESOLUTIONINFO_TAG
{
    uint8_t  imft_std;/*0: PVI 1: CVI 2: HDA 3: HDT */
    uint8_t  imft_res;/*0: SD 480I, 1: SD 576I 2: 720P 3: 1080P 4: 960P or 800P*/
    uint8_t  imft_ref;/*0: 25 HZ, 1: 30 HZ 2: 50 HZ 3: 60HZ*/
    uint16_t width;
    uint16_t height;
    uint16_t framerate;
    uint16_t interlaced;
} PR2K_RESOLUTIONINFO;

typedef enum PR2000_RESOLUTION_ID_TAG
{
    PR2000_CVBS_NTSC = 0,
    PR2000_CVBS_PAL,
    PR2000_HDA960_576I_50FPS,
    PR2000_HDA720P_25FPS,
    PR2000_HDA720P_30FPS,
    PR2000_HDA1080P_25FPS,
    PR2000_HDA1080P_30FPS,
    PR2000_MAX_ID,
} PR2000_RESOLUTION_ID;

#define PR2000_UNDECT 0x0
#define PR2000_DECT   0x1
#define PR2000_LOCK   0x2


#define REG0_DET_IMFT_STD_MASK 0xC0
#define REG0_DET_IMFT_STD_SHIF 6
#define REG0_DET_IMFT_REF_MASK 0x30
#define REG0_DET_IMFT_REF_SHIF 4
#define REG0_DET_IMFT_RES_MASK 0x07
#define REG0_DET_IMFT_RES_SHIF 0

//X10LightDriver_t1.h
typedef struct PR2000SensorDriverStruct *PR2000SensorDriver;
SensorDriver PR2000SensorDriver_Create();
static void PR2000SensorDriver_Destory(SensorDriver base);
void PR2000Initialize(uint16_t Mode);
void PR2000Terminate(void);
void PR2000OutputPinTriState(uint8_t flag);
uint16_t PR2000GetProperty(MODULE_GETPROPERTY property);
uint8_t PR2000GetStatus(MODULE_GETSTATUS Status);
void PR2000PowerDown(uint8_t enable);
uint8_t PR2000IsSignalStable(uint16_t Mode);
//end of X10LightDriver_t1.h

#ifdef __cplusplus
}
#endif

#endif