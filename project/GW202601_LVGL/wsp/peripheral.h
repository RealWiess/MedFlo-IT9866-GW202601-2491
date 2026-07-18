#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include "../scene_utility.h"

// command
typedef enum
{
    PERICMD_NONE,
    PERICMD_GET_ADC_TEMP,
    PERICMD_SET_ADC_TEMP,
    PERICMD_SET_SARADC_LIGHT_SENSOR,
    PERICMD_SET_EEPROM_OK_RESULT,
    PERICMD_SET_EEPROM_NG_RESULT,
    //PERICMD_SET_BACKLIGHT,
    //PERICMD_BURN_IN_MODE,
} PeriCmdID;

typedef struct
{
    PeriCmdID   id;
    int arg1;
    int arg2;
} PeriCmd;

int PeripheralReceive(PeriCmd *cmd);
int PeripheralSend(PeriCmd *cmd);
bool updatePeripheral(void);
uint32_t getPeriInfo(SOC_PERIPHERAL id);

// backlight interface.
bool changeBacklight(unsigned int, int);
bool backlightWithAls;

void VideoSensorInit(void);

bool getVideoSensorSignalStatus(void);
bool getVideoSensorSignalLoss(void);

#endif
