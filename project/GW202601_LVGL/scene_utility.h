#ifndef SCENE_UTILITY_H
#define SCENE_UTILITY_H

typedef enum _MCU_BUTTON {
    BTN_NONE,
    BTN_ENTER = 1,
    BTN_HOME,
    BTN_UP,
    BTN_DOWN,
    BTN_CANCEL,
    // weird button type.
    ABS_IN
} MCU_BUTTON;

typedef enum _MCU_CANDATA {
    CAN_VCUGeneral1 = 1,// 0x100
    CAN_VCUGeneral2,    // 0x101
    CAN_VCUInfo3,       // 0x151
    CAN_VCUInfo1,       // 0x200
    CAN_VCUInfo2,       // 0x201
    CAN_VCUFaultInfo,   // 0x204
    CAN_VCUSettingDash, // 0x350
    CAN_VCUBatPackType, // 0x3A1
    CAN_TPMSInfo1,      // 0x3C0
    CAN_TPMSInfo2,      // 0x3C1
    CAN_VCUTBoxInfo,    // 0x3A0
    CAN_VCUNM,          // 0x400
    CAN_THER,           //0x02
    CAN_BATTERY,        //0x03
} MCU_CANDATA;

typedef enum _SOC_PERIPHERAL {
    PERI_ADC = 1,
    PERI_ALS,
    PERI_EEPROM_OK,
    PERI_EEPROM_NG,
    PERI_UART,
} SOC_PERIPHERAL;

typedef enum _VALUE_CHANGE {
    VAL_ODO = 1,
    VAL_AVGPOWER,
    VAL_TRIP1,
    VAL_TRIP2,
    VAL_AVG1,
    VAL_AVG2,
    VAL_PANEL,
    VAL_RESET
} VALUE_CHANGE;

typedef struct _CustomEventValue {
    MCU_BUTTON buttonEvent;
    MCU_CANDATA candataEvent;
    SOC_PERIPHERAL periEvent;
    VALUE_CHANGE   valueEvent;
} CustomEventValue;

void getCustomEventValue(CustomEventValue *st);
int sceneInjectCustomEvent(int targetCustomEvent, int value);
unsigned int sceneParameter2Value(char *param);
void wiessSceneRunHandler(uint32_t tick);
void setTestData(uint8_t data_id,uint32_t data);
uint8_t getTherDetAlert(void);


#endif
