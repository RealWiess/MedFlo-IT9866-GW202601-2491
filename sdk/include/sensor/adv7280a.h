#ifndef __ADV7280_H__
#define __ADV7280_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_I2P_FUNCTION  1
//#define ENABLE_DEBUG_VIDEO_COLORBAR  1
typedef enum _ADV7280_INPUT_MODE
{
    ADV7280_INPUT_CVBS   = 0,
    ADV7280_INPUT_SVIDEO = 1,
    ADV7280_INPUT_YPBPR  = 2,
} ADV7280_INPUT_MODE;

//sel input channel and format
typedef enum _ADV7280_SEL_CHANNEL
{
    CVBS_AN1       = 0,
    CVBS_AN2       = 1,
    CVBS_AN3       = 2,
    CVBS_AN4       = 3,
    Y_AN1_C_AN2    = 8,
    Y_AN3_C_AN4    = 9,  
    Y_AN1_PB_AN2_PR_AN3    = 12,
} ADV7280_SEL_CHANNEL;

typedef enum _ADV7280_INPUT_STANDARD
{
    ADV7280_NTSC_M_J          = 0x0,
    ADV7280_NTSC_4_43         = 0x1,
    ADV7280_PAL_M             = 0x2,
    ADV7280_PAL_60            = 0x3,
    ADV7280_PAL_B_G_H_I_D     = 0x4,
    ADV7280_SECAM             = 0x5,
    ADV7280_PAL_COMBINATION_N = 0x6,
    ADV7280_SECAM_525         = 0x7,
} ADV7280_INPUT_STANDARD;


typedef struct ADV7280SensorDriverStruct *ADV7280SensorDriver;
SensorDriver ADV7280SensorDriver_Create();
void ADV7280SensorDriver_Destory(SensorDriver base);
void ADV7280Initialize(uint16_t Mode);
void ADV7280Terminate(void);
void ADV7280OutputPinTriState(uint8_t flag);
uint16_t ADV7280GetProperty(MODULE_GETPROPERTY property);
uint8_t ADV7280GetStatus(MODULE_GETSTATUS Status);
void ADV7280PowerDown(uint8_t enable);
uint8_t ADV7280IsSignalStable(uint16_t Mode);

#ifdef __cplusplus
}
#endif

#endif