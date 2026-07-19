#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "SDL/SDL.h"
#include "scene.h"
#include "scene_utility.h"
#include "uart_data.h"
#include "mqueue_dashboard.h"
#include "dbWidget.h"
#include "other_id_impl.h"
#include "dashboard_data.h"

/* Forward declarations from wsp/sys_uart.c */
extern void sysUartSend(int item);

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) /* do nothing */
#endif


#ifdef CFG_DEMO_MODE
    int Brightness[6] = { 20, 40, 60, 80, 100, 100 };

    DashboardData_t theDashData = {
        .u8_brightness = 3,   // 設定預設亮度
    };
#else
    DashboardData_t theDashData = {
        .u8_brightness = 4,   // 設定預設亮度
    };
#endif


#define MAINMENU        3
#define SETCLOCK        4
#define ENGINEERMODE    5



DashboardData_t theDashData;
InternalData_t theInterData = {0};
LayerKeyStatus_t comboKeyStatus = {.layerStatus = DL_DASHBOARD,
                .comboKeyTimer = NULL, .timerIsRunning = false};

static pthread_t canDataTask;
static UartAliveCtrl aliveData = {false, false};
static DBWIGET mileCalcWidget;

static pthread_t cameraTask;//Add Hardy 2025-09-07

void updateSystemMsg(UartMsg_t* msg);
void updateEventMsg(UartMsg_t* msg);
void updateFaultMsg(UartMsg_t* msg);
void updateStatusMsg(UartMsg_t* msg);
void updateValueMsg(UartMsg_t* msg);

static void  updateOdometer(uint16_t speed);

static bool cameraState = false;

void printBoardInfo(void)
{
    printf("\r\n==================================================\r\n");
    printf("Project: %s(%s)\r\nSW Version: v%d.%d.%d.%d\r\nBuildTime: %s %s",
           CFG_HW_VERSION, CFG_SYSTEM_NAME,
           atoi(CFG_FW_VER_MAJOR), atoi(CFG_FW_VER_MINOR),
           atoi(CFG_FW_VER_PATCH), atoi(CFG_FW_VER_CUSTOM),
           __DATE__, __TIME__);
    printf("\r\n==================================================\r\n");
}

void printUartMsg(UartMsg_t* message)
{
    int i;
    size_t len;
    uint32_t cmdId;

    if (!message)
        return;

    len = message->signal.lenBody;
    if (message->signal.controlId == STANDARD_CANBUS_ID)
        cmdId = message->signal.stdId;
    else if (message->signal.controlId == EXTENDED_CANBUS_ID)
        cmdId = message->signal.extId;
    else
        cmdId = message->signal.cmdId;

    printf(">>");
    printf("Control Id %x, Cmd Id  %x, Len %d", message->signal.controlId, cmdId, len);
    for (i = 0; i < len;  ++i)
        printf("Body %x ", message->signal.body[i]);
    printf("<<\n");

}

static void* sysUartAliveTask(void* arg)
{
    // uint8_t cntStop = 0;

    while(1) {
        if (!aliveData.isStopHeartbeat)
            sysUartSend(ITEM_SOC_ALIVE);
        usleep(900000);

        /* for test */
        /*
        cntStop++;
        if (cntStop > 10)
            break;
        */
    }
}

//Add Hardy 2025-09-07
static void* sysUartCameraTask(void* arg)
{
    while (1) {
        //printf("%s ...\n", __func__);
        sysUartSend(ITEM_CAMERA_READY);
        usleep(100 * 1000);
    }
}

void SetBrightness(uint8_t brightness) {
#ifdef CFG_DEMO_MODE
    ithPwmInit(ITH_TIMER1, 1000, Brightness[brightness]);
    ithPwmSetDutyCycle(ITH_TIMER1, Brightness[brightness]);
    //printf("SetBrightness: %d%%\n", Brightness[brightness]);
#else
    //ScreenSetBrightness(4);
    ScreenSetBrightness(brightness);
#endif
}

void updateCanbusData(void)
{
    DashboardEvent_t dbEvt;
    uint32_t* index;

    if (dashboardQueueReceive(&dbEvt)) {
        DEBUG_PRINT("id = %x, len = %d\n", dbEvt.canid, dbEvt.length);

        index = (uint32_t*)(dbEvt.data);
        //printf("temp = %p, dashboardQueueReceive\n", temp);
        /*printf("dbEvt = %d %c, dashboardQueueReceive\n", dbEvt.canid, dbEvt.data);
        for (int i = 0; i < dbEvt.length; i++)
            printf("%x ", index[i]);*/
#ifdef DEBUG
        for(int i = 0; i < dbEvt.length; i++)
            printf("%x ", index[i]);
#endif
        switch (dbEvt.canid) {
        case 0x0:
            theDashData.u16_speed = index[0];
            theDashData.u32_odo = index[1];
            theDashData.u8_gear = index[2];
        case 0x700: //Brightness
            ;
            uint8_t raw_brightness = (index[3] & 0x38) >> 3; 
            //theDashData.u8_brightness = (raw_brightness > 4) ? 4 : raw_brightness;
            theDashData.u8_brightness = (raw_brightness > 5) ? 5 : raw_brightness;
            break;
        /*case 0x3A0:
            theDashData.u8_BAT_SOC = index[7];
            theDashData.u16_timeCHG_Completed = index[8]&0xFFF;
            //printf("(1)0x%x ", theDashData.u16_timeCHG_Completed);
            //theDashData.u16_timeCHG_Completed = byteOrderReverse(&index[8], 2)&0xFFF;
            //printf("(2)0x%x ", theDashData.u16_timeCHG_Completed);
            break;
        case 0x3A1:
            theDashData.u8_BAT_Voltage = index[9];
            break;*/
        default:
            break;
        }

        //ituSceneSendEvent(&theScene, EVENT_CUSTOM_CUART, NULL);
    }
    SetBrightness(theDashData.u8_brightness);
}

void updateMcuData(void)
{
    UartMsg_t message;

    if (uartMsgQueueReceive(&message)) {
        //printUartMsg(&message);
        if (message.signal.controlId == SYSTEM_ID)
        {
            updateSystemMsg(&message);
        }
        else if (message.signal.controlId == EVENT_ID)
        {
            updateEventMsg(&message);
        }
        else if (message.signal.controlId == FAULT_ID)
        {
            updateFaultMsg(&message);
        }
        else if (message.signal.controlId == STATUS_ID)
        {
            updateStatusMsg(&message);
        }
        else if (message.signal.controlId == VALUE_ID)
        {
            updateValueMsg(&message);
        }

        ituSceneSendEvent(&theScene, EVENT_CUSTOM_MUART, NULL);
    }
}

static void* canDataRecvTask(void* arg)
{
    uint32_t timer = SDL_GetTicks();

    while (1) {
        if ((SDL_GetTicks() - timer) > 0) {
            timer++;
            updateCanbusData();
            updateOdometer(theDashData.u8_vehicleSpeed);
        }
    }
}

void initDashboardData(void)
{
    mileCalcWidget = createMileCalcWidget("mileCalcWidget");
    theInterData.mile_set.mile[MT_ODO] = theConfig.odometer;
    theInterData.mile_set.mile[MT_TRIP_A] = theConfig.tripA_meter;

    comboKeyStatus.comboKeyTimer = createLibTimer("comboKey");
    pthread_create(&canDataTask, NULL, canDataRecvTask, NULL);
}

void exitDashboardData(void)
{
    pthread_join(canDataTask, NULL);
    deleteMileCalcWidget(mileCalcWidget);
}

void updateDashboardData(void)
{
    updateMcuData();
}

void resetTrip(MileType type)
{
    if (type != MT_TRIP_A)
        return;

    theInterData.mile_set.mile[type] = 0;
    ConfigSave();
}

void setOdo(uint32_t value)
{
    theInterData.mile_set.mile[MT_ODO] = value;
    ConfigSave();
    deleteMileCalcWidget(mileCalcWidget);
    mileCalcWidget = createMileCalcWidget("mileCalcWidget");
}

static void  updateOdometer(uint16_t speed)
{
    uint32_t mile;
    int i;
    uint32_t realSpeed;

    realSpeed = speed / 2;
    mile = mileCalculator(mileCalcWidget, realSpeed);
    if (mile) {
        for (i = 0; i < MT_MAX; i++) {
            theInterData.mile_set.mile[i] += mile;
        }
        //printf(">> odo = %d(%d)\n", theInterData.mile_set.mile[MT_ODO], mile);
        theConfig.odometer = theInterData.mile_set.mile[MT_ODO];
        theConfig.tripA_meter = theInterData.mile_set.mile[MT_TRIP_A];
        ConfigSave();
    }
}

void updateSystemMsg(UartMsg_t* msg)
{
    pthread_t task;
    pthread_attr_t attr;
    uint8_t cmdId = msg->signal.cmdId;

    if (cmdId == ID_LAST_ALIVE_STATUS) {
        if (!aliveData.hasReceivedLastAlive) {
            printf("Last Status is 0x%02x\n", msg->signal.body[0]);
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&task, &attr, sysUartAliveTask, NULL);
            pthread_create(&cameraTask, NULL, sysUartCameraTask, NULL);//Add Hardy 2025-09-07
            aliveData.hasReceivedLastAlive = true;
        }
    }
}

uint8_t calcBtnHash(uint32_t id, uint8_t val)
{
    uint8_t ret;

    //printf("[%s] id=0x%x, val=0x%d\n", __func__, id, val);
    if (val == 0x20)
        val -= 0x1C; // val = 0x4
    ret= ((id - 0x10) | (val << 4));
    return ret;
}

static void ComboKey(timer_t timerid, int arg)
{
#if 0
    printf("[%s] arg=%x, layer=%d", __func__, arg, comboKeyStatus.layerStatus);
    printf(" bk=%d, mk=%d(running=%d)\n", comboKeyStatus.boostKeyStatus, comboKeyStatus.mileKeyStatus, comboKeyStatus.timerIsRunning);

    if (comboKeyStatus.boostKeyStatus == true && comboKeyStatus.mileKeyStatus == true) {
        if (comboKeyStatus.layerStatus == DL_DASHBOARD) {
            if (theInterData.mile_set.type == MT_ODO && theDashData.u16_ev_speed == 0) {
                SceneGotoLayer(ENGINEERMODE);
            }
        } else if (comboKeyStatus.layerStatus == DL_ENGINEERMODE) {
            SceneGotoLayer(MAINMENU);
        }
    } else if (comboKeyStatus.boostKeyStatus == true) {
        if (comboKeyStatus.layerStatus == DL_DASHBOARD) {
            if (theInterData.mile_set.type == MT_ODO && theDashData.u16_ev_speed == 0) {
                SceneGotoLayer(SETCLOCK);
            }
        } else if (comboKeyStatus.layerStatus == DL_SETCLOCK) {
            SceneGotoLayer(MAINMENU);
        }
    } else {
        //nothing
    }
    comboKeyStatus.boostKeyStatus = false;
    comboKeyStatus.mileKeyStatus = false;
    comboKeyStatus.timerIsRunning = false;
#endif
}

static void setComboKeyTimer(uint32_t keyStatus)
{
    TimerSpec_t timerSpec;

    //printf("[%s] %x \n", __func__, comboKeyStatus.comboKeyTimer);
    if (comboKeyStatus.comboKeyTimer) {
        timerSpec.func = (VOIDFUNCPTR)ComboKey;
        timerSpec.sec = 0;
        timerSpec.nsec = 500000000;
        timerSpec.repeat = 0;
        timerSpec.arg = keyStatus;

        setLibTimer(comboKeyStatus.comboKeyTimer, &timerSpec);
    }
}

void updateEventMsg(UartMsg_t* msg)
{
    uint8_t hash;
    uint32_t odo;
    switch (msg->signal.cmdId) {
    case ID_EVT_PANEL_SET:
        break;
    case ID_EVT_ODO_SET:
        odo = msg->signal.body[0];
        setOdo(odo);
        break;
    case ID_EVT_KEY_DOWN:
#ifdef CFG_DEMO_MODE
        /*Brightness[theDashData.u8_brightness] -=5 ;
        if (Brightness[theDashData.u8_brightness]<5) {
            Brightness[theDashData.u8_brightness] = 5;
        }*/
        break;
#endif
    case ID_EVT_KEY_UP:
#ifdef CFG_DEMO_MODE
        /*Brightness[theDashData.u8_brightness] += 5;
        if (Brightness[theDashData.u8_brightness] > 100) {
            Brightness[theDashData.u8_brightness] = 100;
        }*/
        break;
#endif
    case ID_EVT_KEY_ENTER:
    case ID_EVT_KEY_EXIT:
        hash = calcBtnHash(msg->signal.cmdId, msg->signal.body[0]);
        sceneInjectCustomEvent(EVENT_CUSTOM_BUTTON, hash);
        break;
    }
#if 0
    if (KS_PRESS == msg->signal.body[0]) {
        sceneInjectCustomEvent(EVENT_CUSTOM_BUTTON, msg->signal.cmdId);
        printf("button: 0x%x\n", msg->signal.cmdId);
    }
#endif
}

void updateFaultMsg(UartMsg_t* msg)
{
}

void updateStatusMsg(UartMsg_t* msg)
{
}

void updateValueMsg(UartMsg_t* msg)
{
    uint8_t cmdId = msg->signal.cmdId;
    uint32_t body = msg->signal.body[0] | (msg->signal.body[1] << 8);

    if (cmdId == ID_MCU_FW_VER) {
        printf("MCU f/w version received : %02d.%02d.%02d.%02d\n",
            msg->signal.body[0], msg->signal.body[1],
            msg->signal.body[2], msg->signal.body[3]);
    }/* else if (cmdId == ID_BATT_SEN) {
        interData.batt_sen = body;
        //printf("batt_sen: 0x%x\n", interData.batt_sen);
    } else if (cmdId == ID_THER_DET) {
        interData.ther_det = body;
        //printf("ther_det: 0x%x\n", interData.ther_det);
    }*/
}

MileType getMileState(uint32_t *mile)
{
    *mile = theInterData.mile_set.mile[theInterData.mile_set.type];
    return theInterData.mile_set.type;
}

MileType updateMileState(uint32_t *mile)
{
    theInterData.mile_set.type = (theInterData.mile_set.type + 1) % MT_MAX;
    return getMileState(mile);
}

