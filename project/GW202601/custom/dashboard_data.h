#ifndef DASHBOARD_DATA_H
#define DASHBOARD_DATA_H

#include "dbTimer.h"

#define HASH_ENTER_SINGLE   0x10
#define HASH_UP_SINGLE      0x11
#define HASH_DOWN_SINGLE    0x12

#define HASH_ENTER_3LONG    0x30
#define HASH_UP_3LONG       0x31
#define HASH_DOWN_3LONG     0x32

#define HASH_UP_RELEASE     0x41
#define HASH_DOWN_RELEASE   0x42

#define HASH_ENTER_5LONG    0x50
#define HASH_UP_5LONG       0x51

#define HASH_UP_10LONG      0xA1

#define PWR_MODE_ECO        0x41
#define PWR_MODE_COMFORT    0x42
#define PWR_MODE_SPORT      0x43
#define PWR_MODE_TURBO      0x44
#define PWR_MODE_LOWPWR     0x40

#ifdef CFG_DEMO_MODE
extern int Brightness[6];
#endif
static bool IsCaptureStable;

typedef enum {
    ID_EVT_PANEL_SET = 0x8,
    ID_EVT_ODO_SET = 0x9,
    ID_EVT_KEY_ENTER = 0x10,
    ID_EVT_KEY_UP = 0x11,
    ID_EVT_KEY_DOWN = 0x13,
    ID_EVT_KEY_EXIT = 0x12,
    ID_EVT_MAX,
} EVENT_ID_TYPE;

typedef enum {
    ID_STS_LED_LEFT_TURN = 0x10,
    ID_STS_LED_HEADLIGHT = 0x11,
    ID_STS_LED_MAINTENANCE = 0x12,
    ID_STS_LED_WARNING = 0x13,
    ID_STS_LED_REFILL = 0x14,
    ID_STS_LED_RIGHT_TURN = 0x15,
    ID_STS_MAX,
} STATUS_ID_TYPE;

typedef enum _MileType {
    MT_ODO,
    MT_TRIP_A,
    MT_MAX,
} MileType;

typedef enum _DashboardType {
    DT_FAIL = 0,
    DT_VFC,
    DT_IFC,
    DT_TFC,
    DT_VBUS,
    DT_IBUS,
    DT_PSI,
    DT_PCMD,
    DT_BCMD,
    DT_CHARGE,
    DT_MODE,
    DT_STATE,
    DT_FG,
    DT_ERROR,
    DT_WL,
    DT_RDY,
    DT_MAINT,
    DT_BRAKE,
    DT_LTURN,
    DT_RTURN,
    DT_BROKEN,
    DT_FCC,
    DT_SPEED,
} DashboardType_e;

typedef enum _DashboadLayer_e {
    DL_DASHBOARD,
    DL_SETCLOCK,
    DL_ENGINEERMODE,
} DashboadLayer_e;

typedef enum _GearStatus_e {
    GS_NEUTRAL,
    GS_FORWARD,
    GS_REVERSE,
} GearStatus_e;

typedef enum _DrivingMode_e {
    DM_STANDBY,
    DM_NORMAL,
    DM_SPORT,
    DM_ECO,
    DM_CHARGING,
} DrivingMode_e;

typedef enum _PowerMode_e {
    PM_OFF,
    PM_RESERVE,
    PM_READY,
    PM_RUN,
} PowerMode_e;

typedef struct _DashboardData {
    uint16_t u16_speed;
    uint32_t u32_odo;
    uint8_t u8_gear;
    uint8_t u8_vehicleSpeed;
    uint8_t u8_gearStatus;
    uint8_t u8_drivingMode;
    bool b_workMode;
    uint8_t u8_powerMode;
    bool b_turtleLight;
    bool b_tcsLight;
    bool b_absLight;
    uint8_t u8_BAT_SOC;
    uint16_t u16_timeCHG_Completed;
    uint8_t u8_BAT_Voltage;
    uint8_t u8_brightness; // Default brightness level
} DashboardData_t;

typedef struct _layerKeyStatus {
    bool mileKeyStatus;
    bool boostKeyStatus;
    DashboadLayer_e layerStatus;
    LIBTIMER comboKeyTimer;
    bool timerIsRunning;
} LayerKeyStatus_t;

typedef struct _MileSet {
    MileType type;
    uint32_t mile[MT_MAX];
} MileSet;

typedef struct _InternalData {
    uint32_t batt_sen;
    uint32_t ther_det;
    MileSet mile_set;
} InternalData_t;



void printBoardInfo(void);

void initDashboardData(void);
void updateDashboardData(void);
void exitDashboardData(void);

MileType getMileState(uint32_t *mile);
MileType updateMileState(uint32_t *mile);
void resetTrip(MileType type);
void setOdo(uint32_t value);

extern DashboardData_t theDashData;
extern InternalData_t theInterData;
extern LayerKeyStatus_t comboKeyStatus;

#endif
