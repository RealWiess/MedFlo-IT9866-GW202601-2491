#include <sys/ioctl.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "ite/itp.h"
#include "uart_data.h"
#include "mqueue_uart_in.h"
#include "mqueue_uart_out.h"

#ifdef CFG_UART1_ENABLE

#ifndef _WIN32
#include "uart/uart.h"
#else
#define WIN32_COM               6
#endif

#define SYSUART_BAUDRATE        CFG_UART1_BAUDRATE
#define SYSUART_GPIO_TX         CFG_GPIO_UART1_TX
#define SYSUART_GPIO_RX         CFG_GPIO_UART1_RX
#define SYSUART_PORT            ITP_DEVICE_UART1
#define SYSUART_ITH_PORT        ITH_UART1
#define SYSUART_DEVICE          itpDeviceUart1

#define BUFFER_SIZE             1024
#define SEND_BUF_SIZE           64

static sem_t sysUartSem;
static pthread_t recvTask;
static pthread_t parseTask;
static pthread_t storeTask;
static pthread_t rtcTask;
static volatile bool extQuit = false;

extern void* sysUartParseTask(void* arg);
extern void packUartMsg(OTHER_ID_ITEM item, UartMsg_t* uartMsg);
void sysUartSend(OTHER_ID_ITEM item);

static void sysUartCallback(void* arg1, uint32_t arg2) {
    sem_post(&sysUartSem);
}

static inline void initPhyUart() {
    // SoC to MCU
#ifndef _WIN32
    UART_OBJ sysUartInitParam = {};

    sysUartInitParam.port = SYSUART_ITH_PORT;
    sysUartInitParam.parity = ITH_UART_NONE;
    sysUartInitParam.txPin = SYSUART_GPIO_TX;
    sysUartInitParam.rxPin = SYSUART_GPIO_RX;
    sysUartInitParam.baud = 460800;
    sysUartInitParam.timeout = 0;
    sysUartInitParam.mode = UART_INTR_MODE;
    sysUartInitParam.forDbg = false;

    ioctl(SYSUART_PORT, ITP_IOCTL_RESET, (void*)&sysUartInitParam);
    ioctl(SYSUART_PORT, ITP_IOCTL_REG_UART_CB, (void*)sysUartCallback);
    printf("start uart interrupt mode\n");
    sem_init(&sysUartSem, 0, 0);
#endif
}

static void* sysUartRecvTask(void* arg)
{
    printf("%s ...\n", __func__);
    uartInStruct uartInData;
    uartOutStruct uartOutData;
    while (!extQuit) {
        sem_wait(&sysUartSem);

        // Receive command for sending uart data
        if (uartOutQueueReceive(&uartOutData) > 0) {
#ifndef _WIN32
            write(SYSUART_PORT, &uartOutData, BUFFER_SIZE);
#else
            UartSend(WIN32_COM, &uartOutData, BUFFER_SIZE);
#endif
        }

        memset(&uartInData, 0, sizeof(uartInStruct));
        // Read data from UART port
#ifndef _WIN32
        uartInData.readLen = read(SYSUART_PORT, uartInData.inDataBuf, BUFFER_SIZE);
#else
        uartInData.readLen = UartReceive(WIN32_COM, uartInData.inDataBuf, BUFFER_SIZE);
#endif

        if (uartInData.readLen) {
            uartInQueueSend(&uartInData);
            usleep(10000);
        } /*else {
            usleep(10000);
        }*/
    }
    return NULL;
}

static void* sysUartStoreTask(void* arg)
{
    //printf("%s ...\n", __func__);
#if 0
    while (!extQuit) {
        if (canMsgQueueRecv()
) {
            usleep(1000);
        } else {
            usleep(10000);
        }
    }
#endif
    return NULL;
}

static void* sysUartRtcTask(void* arg)
{
    while(1) {
        //printf("%s ...%d\n", __func__, SDL_GetTicks());
#if 0
        sysUartSend(ITEM_ODO_VALUE);
        sysUartSend(ITEM_RTC);
#endif
        usleep(500000);
    }
}


void sysUartSend(OTHER_ID_ITEM item)
{
    size_t len;
    UartMsg_t uartMsg = {0};
    uint8_t outDataBuf[SEND_BUF_SIZE] = {0};

    memset(&uartMsg, 0, sizeof(UartMsg_t));
    packUartMsg(item, &uartMsg);
    len = packOutputData(outDataBuf, &uartMsg);

#ifndef _WIN32
        write(SYSUART_PORT, outDataBuf, len);
#else
        UartSend(WIN32_COM, outDataBuf, len);
#endif
}

void SysUartInit(void)
{
    uartMsgQueueInit();

#ifdef _WIN32
    if(ComInit(WIN32_COM, SYSUART_BAUDRATE)) {
        //ComInit Error
        printf("Can't open Win 32 COM...\n");
        return;
    }
#else
    initPhyUart();
#endif
    uartInQueueInit();
    uartOutQueueInit();

#if 0   // for test
    extern void heartBeatCallback(void *arg);
    extern void setHeartBeatStop(void *arg);
    timer_init(heartBeatCallback, 0, 900, "heart beat");
    timer_init(setHeartBeatStop, 9000, 0, "t");
#endif
#if 0 // TODO
    DashBluetoothStruct data = {0x00, 0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0A};
    setx321Data2(&data);
    timer_init(dash500msCallback, 0, 500, "dash 500ms");

    timer_init(dash500msCallback, 0, 500, "dash 500ms");
#endif

    pthread_create(&recvTask, NULL, sysUartRecvTask, NULL);
    pthread_create(&parseTask, NULL, sysUartParseTask, NULL);
    pthread_create(&storeTask, NULL, sysUartStoreTask, NULL);
    pthread_create(&rtcTask, NULL, sysUartRtcTask, NULL);
    sysUartSend(ITEM_SOC_FW_VER);
}

void SysUartExit(void) {
    extQuit = true;
    pthread_join(recvTask, NULL);
    pthread_join(parseTask, NULL);
    pthread_join(storeTask, NULL);
    pthread_join(rtcTask, NULL);

    uartInQueueExit();
    uartOutQueueExit();
}

#else 

void sysUartSend(OTHER_ID_ITEM item) { }
void SysUartInit(void) { }
void SysUartExit(void) { }

#endif
