#ifndef UART_DATA_H
#define UART_DATA_H

#include "mqueue_uart_msg.h"
#include "dashboard_data.h"

#if 0
typedef enum {
    ID_KEY = 0x01,
    ID_THER_DET,
    ID_BATTERY_SEN,
    ID_MCU_ADC_CH2,
} OTHER_ID_TYPE;
#endif

typedef enum {
    /* MCU to SoC */
    ID_LAST_ALIVE_STATUS = 0x05,
    /* SoC to MCU */
    ID_ALIVE_STATUS = 0x84,
    ID_POWER_CONTROL = 0x87,
    ID_CAMERA_READY = 0x95,
} SYSTEM_ID_TYPE;

/*
typedef enum {
} FAULT_ID_TYPE;
*/

typedef enum {
    /* MCU to SOC */
    ID_MCU_FW_VER = 0x1,
    ID_BATT_SEN = 0x2,
    ID_THER_DET = 0x3,
    /* SOC to MCU */
    ID_SOC_FW_VER = 0x88,
    ID_LIGHT_SENSOR = 0x91,
} VALUE_ID_TYPE;


typedef enum _CONTROL_ID_TYPE {
    STANDARD_CANBUS_ID  = 0,
    EXTENDED_CANBUS_ID,
    REVERSED,
    SYSTEM_ID,
    EVENT_ID,
    FAULT_ID,
    STATUS_ID,
    VALUE_ID,
/*
    GPIO_ID             = 2,    // SoC to MCU
    OTHER_ID            = 3,    // MCU to SoC
    SYSTEM_ID           = 4,
*/
    MAX_ID,
} CONTROL_ID_TYPE;

typedef enum {
    KS_RELEASE,
    KS_PRESS,
    KS_DOUBLE,
    KS_LONG_PRESS_3,
    KS_LONG_PRESS_5 = 5,
    KS_LONG_PRESS_10 = 10,
} KEY_STATUS;

typedef enum {
    LS_OFF,
    LS_ON,
    LS_SHORT_FLASH,
    LS_LONG_FLASH,
} LED_STATUS;

typedef enum _STANDARD_CANBUS_ID_ITEM{
    ITEM_DASH_BLUETOOTH = 0,
    ITEM_DASH_MAINT_INFO,
    ITEM_DASH_ODO_INFO,
    ITEM_DASH_SETTING,
    ITEM_DASH_TRIP1_INFO,
    ITEM_DASH_TRIP2_INFO,
    ITEM_DASH_VER_NUMBER,
    ITEM_DASH_VEHICLE_SPEED
} STANDARD_CANBUS_ID_ITEM;

typedef enum _OTHER_ID_ITEM {
    /* system */
    ITEM_SOC_ALIVE,
    ITEM_SOC_PWR_CTRL,
    ITEM_CAMERA_READY,//Add Hardy 2025-09-07
    /* event */
    /* fault */
    /* status */
    ITEM_LED_LEFT_TURN,
    ITEM_LED_HEADLIGHT,
    ITEM_LED_MAINTENANCE,
    ITEM_LED_WARNING,
    ITEM_LED_REFILL,
    ITEM_LED_RIGHT_TURN,
    /* value */
    ITEM_LIGHT_SENSOR,
    ITEM_SOC_FW_VER,
    ITEM_ODO_VALUE,
    ITEM_RTC,
    ITEM_MAX,
} OTHER_ID_ITEM;

typedef struct _UartMsgAttr {
    size_t len;
    CONTROL_ID_TYPE ctrl;
    uint8_t cmd;
    uint8_t body;
} UartMsgAttr;

typedef struct _UartAliveCtrl {
    bool isStopHeartbeat;
    bool hasReceivedLastAlive;
} UartAliveCtrl;

#endif
