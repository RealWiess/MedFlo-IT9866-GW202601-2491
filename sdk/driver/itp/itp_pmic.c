/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL WatchDog functions.
 *
 * @author
 * @version 1.0
 */
#include <errno.h>
#include <time.h>
#include "itp_cfg.h"

#define PMIC_SLAVE_ADDR     0x46
#define WD_CTRL_REG1        0x0B

static bool wdEnabled;
static int gMasterDev = 0;
static uint8_t FeedWD[2] = {0};
static pthread_mutex_t wdMutex;


#ifdef CFG_PMIC_WATCHDOG_IDLETASK
static uint32_t wdLastTick = 0;
#endif


static void
PmicRefreshHandler (
    timer_t timerid,
    int     arg)
{
    ITPI2cInfo evt;
    bool iic_result = false;
    
    if (wdEnabled)
    {
        pthread_mutex_lock(&wdMutex);
        
        // Feed WD food
        evt.slaveAddress = PMIC_SLAVE_ADDR;
        evt.cmdBuffer = FeedWD;
        evt.cmdBufferSize  = 2;
        
        iic_result = write(gMasterDev, &evt, 1);

        pthread_mutex_unlock(&wdMutex);
    }
}

#ifdef CFG_PMIC_WATCHDOG_IDLETASK

static void
PmicIdleRefreshHandler (
    void)
{
    if (itpGetTickDuration(wdLastTick) >= CFG_PMIC_WATCHDOG_REFRESH_INTERVAL)
    {
        PmicRefreshHandler(0, 0);
        wdLastTick = itpGetTickCount();
    }
}

#endif // CFG_PMIC_WATCHDOG_IDLETASK

static void PmicWDEnable(
    int timeout)
{
    ITPI2cInfo evt;
    uint8_t recvBuffer[1] = {0};
    bool iic_result = false;

    pthread_mutex_lock(&wdMutex);

    FeedWD[0] = WD_CTRL_REG1;

    if (timeout == 1250) {
        // timeout set to 1250 ms, and enable WD
        FeedWD[1] = 0x95;
    }
    else if (timeout == 2500) {
        // timeout set to 2500 ms, and enable WD
        FeedWD[1] = 0x96;
    }
    else if (timeout == 5000) {
        // timeout set to 5000 ms, and enable WD
        FeedWD[1] = 0x17;
    }
    else if (timeout == 0) {
        // timeout set to 39 ms, and enable WD
        FeedWD[1] = 0x90;
    }
    else {
        ithPrintf("[%s] NOT allow timeout(%d). Use default timeout 5000 ms\n", __FUNCTION__, timeout);
        FeedWD[1] = 0x17;
    }
  
    // PMIC enable WD function, and set timeout
    evt.slaveAddress = PMIC_SLAVE_ADDR;
    evt.cmdBuffer = FeedWD;
    evt.cmdBufferSize  = 2;
    evt.dataBuffer     = recvBuffer;
    evt.dataBufferSize = 1;
    
    iic_result = write(gMasterDev, &evt, 1);
    
    if (iic_result != 0)
    {
        ithPrintf("[%s] I2c Write Error\n", __FUNCTION__);
    }

#if 1       
    iic_result = read(gMasterDev, &evt, 1);
    
    if (iic_result < 0)
    {
        ithPrintf("[%s] IIC read failed!\n", __FUNCTION__);
    }
    else
    {
        ithPrintf("[%s] PSQ9905(WD_CTRL_REG1): %02x\n", __FUNCTION__, recvBuffer[0]);
    }
#endif

    pthread_mutex_unlock(&wdMutex);
}

static void PmicWDDisable(
    int timeout)
{
    ITPI2cInfo evt;
    uint8_t recvBuffer[1] = {0};
    uint8_t sendBuffer[2] = {0};
    bool iic_result = false;

    pthread_mutex_lock(&wdMutex);
    
    sendBuffer[0] = WD_CTRL_REG1;

    if (timeout == 1250) {
        // timeout set to 1250 ms, and disable WD
        sendBuffer[1] = 0x05;
    }
    else if (timeout == 2500) {
        // timeout set to 2500 ms, and disable WD
        sendBuffer[1] = 0x06;
    }
    else if (timeout == 5000) {
        // timeout set to 5000 ms, and disable WD
        sendBuffer[1] = 0x87;
    }
    else if (timeout == 0) {
        // timeout set ot 39 ms, and disable WD
        sendBuffer[1] = 0x00;
    }
    else {
        ithPrintf("[%s] NOT allow timeout(%d). Use default timeout 5000 ms\n", __FUNCTION__, timeout);
        sendBuffer[1] = 0x87;
    }

    // PMIC disable WD
    evt.slaveAddress = PMIC_SLAVE_ADDR;
    evt.cmdBuffer = sendBuffer;
    evt.cmdBufferSize  = 2;
    evt.dataBuffer     = recvBuffer;
    evt.dataBufferSize = 1;
    
    iic_result = write(gMasterDev, &evt, 1);
    
    if (iic_result != 0)
    {
        ithPrintf("[%s] I2c Write Error\n", __FUNCTION__);
    }

#if 1    
    iic_result = read(gMasterDev, &evt, 1);
    
    if (iic_result < 0)
    {
        ithPrintf("[%s] IIC read failed!\n", __FUNCTION__);
    }
    else
    {
        ithPrintf("[%s] PSQ9905(WD_CTRL_REG1): %02x\n", __FUNCTION__, recvBuffer[0]);
    }
#endif

    pthread_mutex_unlock(&wdMutex);
}

static void
PmicWDGetCtrl (
    void)
{
    ITPI2cInfo evt;
    uint8_t cmd = 0x0;    
    bool iic_result = false;
    uint8_t WD_InitVaule = 0x0;
    
    // read status from WD Ctrl register1
    cmd = WD_CTRL_REG1;

    evt.slaveAddress = PMIC_SLAVE_ADDR;
    evt.cmdBuffer = &cmd;
    evt.cmdBufferSize = 1;
    evt.dataBuffer = &WD_InitVaule;
    evt.dataBufferSize = 1;
    
    iic_result = read(gMasterDev, &evt, 1);

    if (iic_result < 0) {
        ithPrintf("[%s] IIC read failed!\n", __FUNCTION__);
    }
    else {
        ithPrintf("[%s] PSQ9905(WD_CTRL_REG1): %02x\n", __FUNCTION__, WD_InitVaule);
    }
}

static void
PmicWDInit (
    void)
{    
    wdEnabled = false;
    pthread_mutex_init(&wdMutex, NULL);

#if defined(CFG_PMIC_I2C0)
    gMasterDev = open(":i2c0", 0);
#elif defined(CFG_PMIC_I2C1)
    gMasterDev = open(":i2c1", 0);
#elif defined(CFG_PMIC_I2C2)
    gMasterDev = open(":i2c2", 0);
#elif defined(CFG_PMIC_I2C3)
    gMasterDev = open(":i2c3", 0);
#endif

    // read status from WD Ctrl register1
    PmicWDGetCtrl();

#if (defined(CFG_PMIC_WATCHDOG_IDLETASK)) && (CFG_PMIC_WATCHDOG_REFRESH_INTERVAL > 0)

    itpRegisterIdleHandler(PmicIdleRefreshHandler);

#elif CFG_PMIC_WATCHDOG_REFRESH_INTERVAL > 0
    timer_t             timer   = 0;
    struct itimerspec   value;
    int                 sec     = CFG_PMIC_WATCHDOG_REFRESH_INTERVAL / 1000;
    int                 m_sec   = CFG_PMIC_WATCHDOG_REFRESH_INTERVAL % 1000;

    (void)timer_create(CLOCK_REALTIME, NULL, &timer);
    (void)timer_connect(timer, (VOIDFUNCPTR)&PmicRefreshHandler, 0);
    value.it_interval.tv_sec = sec;
    value.it_value.tv_sec = value.it_interval.tv_sec;
    value.it_interval.tv_nsec = m_sec * 1000000;
    value.it_value.tv_nsec = value.it_interval.tv_nsec;
    (void)timer_settime(timer, 0, &value, NULL);
#endif

}

static int
PmicIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    int ret = 0;

    switch (request)
    {
        case ITP_IOCTL_INIT:
            PmicWDInit();
            break;

        case ITP_IOCTL_ENABLE:
            PmicWDEnable(CFG_PMIC_WATCHDOG_TIMEOUT);
            wdEnabled = true;
            break;

        case ITP_IOCTL_DISABLE:
            PmicWDDisable(CFG_PMIC_WATCHDOG_TIMEOUT);
            wdEnabled = false;
            break;

        case ITP_IOCTL_PMIC_REBOOT:
            wdEnabled = false;
            // reboot right now
            PmicWDEnable(0);
            break;

        case ITP_IOCTL_PMIC_GET:
            ithPrintf("[%s] ITP_IOCTL_PMIC_GET, wdEnabled=%d\n", __FUNCTION__);
            PmicWDGetCtrl();
            break;

        default:
            errno = (int)(((uint32_t)ITP_DEVICE_PMIC << ITP_DEVICE_ERRNO_BIT) | (uint32_t)__LINE__);
            ret = -1;
            break;
    }
    return ret;
}

const ITPDevice itpDevicePmicWD =
{
    ":pmic",
    &itpOpenDefault,
    &itpCloseDefault,
    &itpReadDefault,
    &itpWriteDefault,
    &itpLseekDefault,
    &PmicIoctl,
    NULL
};
