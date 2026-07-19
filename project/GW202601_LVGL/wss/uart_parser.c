#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "mqueue_uart_in.h"
#include "uart_data.h"
#include "dashboard_data.h"
#include "dbWidget.h"
#include "other_id_impl.h"
#include "can_id_impl.h"
#include "util.h"

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) /* do nothing */
#endif

#define CANBUS_EN
#define SYSUART_EN

#define UartHeader                      0xAA
#define UartHeader2                     0x55
#define BodyLengthPosirion              2
#define ControlIdPosition               3
#define CommandIdPosition               4

#define VcuCanBodyLength                14
#define NucCanBodyLength                7

#define COMMAND_BUFFER_SIZE             28
#define COMMAND_BUFFER_MININUM_SIZE     7
#define PAYLOAD_MAX_SIZE                8

#define CONTROL_ID_LENGTH               1
#define STANDARD_CANBUS_ID_LENGTH       2
#define EXTENDED_CANBUS_ID_LENGTH       4
#define OTHER_ID_LENGTH                 1

typedef enum GET_UART_COMMAND_STATE_TAG {
    GET_HEADER,
    GET_HEADER2,
    GET_LENGTH,
    GET_CONTROL_ID,
    GET_COMMAND_ID,
    GET_PAYLOAD,
    GET_CHECKSUM,
} GET_UART_COMMAND_STATE;

static int tailPointer = 0;
static int commandIDLength = 0;
static GET_UART_COMMAND_STATE gState = GET_HEADER;
static unsigned char dataBuf[COMMAND_BUFFER_SIZE];

#ifdef CFG_CAPTURE_ENABLE
extern bool ithCapDeviceIsSignalStable();//Add Hardy 2025-09-18
#endif

static UartMsgAttr uartMsgAttr[] = {
    [ITEM_SOC_ALIVE]         = {.len = 1, .ctrl = SYSTEM_ID, .cmd = 0x84, .body = 0},
    [ITEM_SOC_PWR_CTRL]      = {.len = 1, .ctrl = SYSTEM_ID, .cmd = 0x87, .body = 0},
    [ITEM_CAMERA_READY] = {.len = 1, .ctrl = SYSTEM_ID, .cmd = 0x95, .body = 0},//Add Hardy 2025-09-07

    [ITEM_LED_LEFT_TURN]     = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x10, .body = 0},
    [ITEM_LED_HEADLIGHT]     = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x11, .body = 0},
    [ITEM_LED_MAINTENANCE]   = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x12, .body = 0},
    [ITEM_LED_WARNING]       = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x13, .body = 0},
    [ITEM_LED_REFILL]        = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x14, .body = 0},
    [ITEM_LED_RIGHT_TURN]    = {.len = 1, .ctrl = STATUS_ID, .cmd = 0x15, .body = 0},

    [ITEM_LIGHT_SENSOR]      = {.len = 2, .ctrl = VALUE_ID, .cmd = 0x91, .body = 0},
    [ITEM_SOC_FW_VER]        = {.len = 4, .ctrl = VALUE_ID, .cmd = 0x88, .body = 0},
    [ITEM_ODO_VALUE]         = {.len = 3, .ctrl = VALUE_ID, .cmd = 0x8E, .body = 0},
    [ITEM_RTC]               = {.len = 5, .ctrl = VALUE_ID, .cmd = 0x95, .body = 0},
};

static void dumpReadInfo(
    const unsigned char* tag,
    const unsigned char* info,
    const int len,
    const int start
) {
    printf("\n------[%s] %d -> %d ------@%ld\n", tag, start, len, SDL_GetTicks());
    // printf("  start %d, read length is %d\n", start, len);
    for (int i = start; i < len; i++) {
        printf("0x%02x ", info[i]);
        if (i > 0 && i % 16 == 0)
            printf("\n");
    }
    printf("\n-------[%s] end dump ---------\n", tag);
}

static inline void classifyCommandID(unsigned char* inDataBuf) {
    switch (inDataBuf[tailPointer]) {
#ifdef CANBUS_EN
    case STANDARD_CANBUS_ID:
        commandIDLength = STANDARD_CANBUS_ID_LENGTH;
        break;
    case EXTENDED_CANBUS_ID:
        commandIDLength = EXTENDED_CANBUS_ID_LENGTH;
        break;
#endif
    case SYSTEM_ID:
    case EVENT_ID:
    case FAULT_ID:
    case STATUS_ID:
        commandIDLength = OTHER_ID_LENGTH;
        break;
    default:
        commandIDLength = 0;
        break;
    }
}

static void parseUartData()
{
    uint8_t controlId = dataBuf[ControlIdPosition];
    int offset = 2 + 1 + CONTROL_ID_LENGTH;  //  Header x2 + Length + Control ID
    int bodyLen = *(dataBuf + 2);
    uint8_t* buf = dataBuf + offset;

    switch (controlId) {
#ifdef CANBUS_EN
    case STANDARD_CANBUS_ID:
        findCanMsgImpl(buf, STANDARD_CANBUS_ID_LENGTH, bodyLen);
        break;
    case EXTENDED_CANBUS_ID:
        findCanMsgImpl(buf, EXTENDED_CANBUS_ID_LENGTH, bodyLen);
        break;
#endif
    case SYSTEM_ID:
    case EVENT_ID:
    case FAULT_ID:
    case STATUS_ID:
    case VALUE_ID:
        findUartMsgImpl(buf, OTHER_ID_LENGTH, bodyLen, controlId);
        break;
    default:
        break;
    }
}

void packUartMsg(OTHER_ID_ITEM item, UartMsg_t* uartMsg)
{
    uint8_t hour, min, day, month;
    uint8_t state;
    uint32_t year;
    uint32_t odo;

    if (!uartMsg || item >= ITEM_MAX) {
        printf("parameter of %s is failed\n", __func__);
        return;
    }
    state = (theInterData.mile_set.type + 1) << 4 | comboKeyStatus.layerStatus + 1;

    uartMsg->signal.lenBody = uartMsgAttr[item].len;
    uartMsg->signal.controlId = uartMsgAttr[item].ctrl;
    uartMsg->signal.cmdId = uartMsgAttr[item].cmd;

    if (item == ITEM_SOC_FW_VER) {
        uartMsg->signal.body[0] = atoi(CFG_FW_VER_MAJOR);
        uartMsg->signal.body[1] = atoi(CFG_FW_VER_MINOR);
        uartMsg->signal.body[2] = atoi(CFG_FW_VER_PATCH);
        uartMsg->signal.body[3] = atoi(CFG_FW_VER_CUSTOM);
    } else if (item == ITEM_ODO_VALUE) {
        odo = theInterData.mile_set.mile[MT_ODO] / 1000;
        uartMsg->signal.body[2] = (odo & 0xFF0000) >> 16;
        uartMsg->signal.body[1] = (odo & 0xFF00) >> 8;
        uartMsg->signal.body[0] = (odo & 0xFF);
    } else if (item == ITEM_RTC) {
        getDigitalClock(&hour, &min, &day, &month, &year);
        uartMsg->signal.body[0] = year - 2000;
        uartMsg->signal.body[1] = month;
        uartMsg->signal.body[2] = day;
        uartMsg->signal.body[3] = hour;
        uartMsg->signal.body[4] = min;
    } else if (item == ITEM_SOC_ALIVE) {
       uartMsg->signal.body[0] = state;
    }
    //Add Hardy 2025-09-07
    else if(item == ITEM_CAMERA_READY)
    {
#ifdef CFG_CAPTURE_ENABLE
        IsCaptureStable = ithCapDeviceIsSignalStable();
        uartMsg->signal.body[0] |= (IsCaptureStable & 0x01);
        uartMsg->signal.body[0] |= ((theDashData.u8_brightness & 0x07) << 1);
        /*printf("body[0] = 0x%02X (CaptureFired=%s, Brightness=%u)\n",
            uartMsg->signal.body[0],
            (IsCaptureStable ? "TRUE" : "FALSE"),
            (theDashData.u8_brightness & 0x07));*/
#endif
    } else {
        uartMsg->signal.body[0] = uartMsgAttr[item].body;
    }
}

size_t packOutputData(uint8_t* outputData, UartMsg_t *uartMsg)
{
    uint8_t i = 0, j = 0;
    uint8_t cmdLen = 0;
    uint8_t dataLen = uartMsg->signal.lenBody;
    uint8_t checkSum = 0;

    if (!outputData || !uartMsg) {
        printf("parameter of %s is failed\n", __func__);
        return 0;
    }

    outputData[i++] = 0xAA;      // Header
    outputData[i++] = 0x55;      // Header

    outputData[i++] = uartMsg->signal.lenBody;
    outputData[i++] = uartMsg->signal.controlId;
    if (STANDARD_CANBUS_ID == uartMsg->signal.controlId) {
        outputData[i++] = (uartMsg->signal.stdId & 0xff00) >> 8;
        outputData[i++] = (uartMsg->signal.stdId & 0x00ff);
        reverseData2(uartMsg->signal.body, 8);
    } else {
        outputData[i++] = uartMsg->signal.cmdId;
    }
    memcpy(&outputData[i], uartMsg->signal.body, dataLen);
    i += dataLen;

    // checksum
    // Control ID + Command ID + Body
    for (j = 3; j < i; j++) {
        checkSum += outputData[j];
    }
    outputData[i++] = (unsigned char)checkSum;
    cmdLen = i;

#ifdef DEBUG_PACKAGE
    printf("\n");
    for (i = 0; i< cmdLen; i++)
        printf("%02X:", outputData[i]);
    printf(":>cmdLen = %d\n", cmdLen);
#endif

    return cmdLen;

}

void* sysUartParseTask(void* arg)
{
    uartInStruct uartInData;
    int payloadCount = 0;
    unsigned char checkSum = 0;
    unsigned char checkSumLen = 0;
    int cmdPos = 0;
    int i = 0;

    memset(&uartInData, 0, sizeof(uartInStruct));
    while(1) {  //todo, when the app is closed, no looping
        if (uartInQueueReceive(&uartInData)) {
            tailPointer = 0;
            while (uartInData.readLen > tailPointer) {
                DEBUG_PRINT("%02x ", uartInData.inDataBuf[tailPointer]);
                switch (gState) {
                case GET_HEADER:
                    if (UartHeader == uartInData.inDataBuf[tailPointer]) {
                        dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                        gState = GET_HEADER2;
                    }
                    break;

                case GET_HEADER2:
                    if (UartHeader2 == uartInData.inDataBuf[tailPointer]) {
                        dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                        gState = GET_LENGTH;
                    } else {
                        cmdPos = 0;
                        memset(dataBuf, 0, COMMAND_BUFFER_SIZE);
                        gState = GET_HEADER;
                    }
                    break;

                case GET_LENGTH:
                    if (PAYLOAD_MAX_SIZE >= uartInData.inDataBuf[tailPointer] && 0 < uartInData.inDataBuf[tailPointer]) {
                        dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                        gState = GET_CONTROL_ID;
                    } else {
                        cmdPos = 0;
                        memset(dataBuf, 0, COMMAND_BUFFER_SIZE);
                        gState = GET_HEADER;
                    }
                    break;

                case GET_CONTROL_ID:
                    if (MAX_ID >= uartInData.inDataBuf[tailPointer]) {
                        dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                        classifyCommandID(uartInData.inDataBuf);
                        gState = GET_COMMAND_ID;
                        checkSumLen++;
                    } else {
                        cmdPos = 0;
                        memset(dataBuf, 0, COMMAND_BUFFER_SIZE);
                        gState = GET_HEADER;
                    }
                    break;

                case GET_COMMAND_ID:
                    payloadCount++;
                    dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                    if (payloadCount < commandIDLength) {
                        gState = GET_COMMAND_ID;
                    } else {
                        gState = GET_PAYLOAD;
                        payloadCount = 0;
                    }
                    break;

                case GET_PAYLOAD:
                    payloadCount++;
                    dataBuf[cmdPos++] = uartInData.inDataBuf[tailPointer];
                    if (payloadCount < dataBuf[BodyLengthPosirion]) {
                        gState = GET_PAYLOAD;
                    } else {
                        gState = GET_CHECKSUM;
                        payloadCount = 0;
                    }
                    break;

                case GET_CHECKSUM:
                    // checkSum is sum of all payloads
                    if (cmdPos == VcuCanBodyLength || cmdPos == NucCanBodyLength ||
                        cmdPos == 6) {
                        checkSumLen = cmdPos - 3; //3:0xaa+0x55+body length (3byte)
                    } else {
                        checkSumLen = cmdPos - 3; //3:0xaa+0x55+body length (3byte)
                    }
                    dataBuf[cmdPos] = uartInData.inDataBuf[tailPointer];
                    for (i = 1; i <= checkSumLen; i++) {
                        checkSum += dataBuf[cmdPos - i];
                    }
                    if (checkSum == dataBuf[cmdPos]) {
                        // Get one command
                        parseUartData();
                    } else {
                        dumpReadInfo("UART", dataBuf, COMMAND_BUFFER_SIZE, 0);
                        printf("[GET_CHECKSUM] Wrong, checkSum!=0x%x, dataBuf=0x%x\n", checkSum, dataBuf[cmdPos]);
                    }
                    cmdPos = 0;
                    checkSum = 0;
                    checkSumLen = 0;
                    memset(dataBuf, 0, COMMAND_BUFFER_SIZE);
                    gState = GET_HEADER;
                    DEBUG_PRINT("\n");
                    break;

                default:
                    break;
                }
                tailPointer++;

            }

            memset(&uartInData, 0, sizeof(uartInStruct));
        }
    }
}

