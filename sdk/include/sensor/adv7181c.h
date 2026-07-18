#ifndef __ADV7181C_H__
#define __ADV7181C_H__

#include "ite/itp.h"
#include "ith/ith_defs.h"
#include "ite/ith.h"
#include "mmp_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum _ADV7181_INPUT_STANDARD
{
	XVGA_1024x768x60 = 0,
	SVGA_800x600x56,
	SVGA_800x600x60,
	VGA_640x480x60,
	SXGA_1280x1024x60, 
} ADV7181_INPUT_STANDARD;


typedef struct ADV7181SensorDriverStruct *ADV7181SensorDriver;
SensorDriver ADV7181SensorDriver_Create();
void ADV7181SensorDriver_Destory(SensorDriver base);
void ADV7181Initialize(uint16_t Mode);
void ADV7181Terminate(void);
void ADV7181OutputPinTriState(uint8_t flag);
uint16_t ADV7181GetProperty(MODULE_GETPROPERTY property);
uint8_t ADV7181GetStatus(MODULE_GETSTATUS Status);
void ADV7181PowerDown(uint8_t enable);
uint8_t ADV7181IsSignalStable(uint16_t Mode);

#ifdef __cplusplus
}
#endif

#endif