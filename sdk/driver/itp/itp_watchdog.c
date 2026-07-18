/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL WatchDog functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <time.h>
#include "itp_cfg.h"
#include "itp_watchdog.h"

#ifdef CFG_NAND_ENABLE
#include "ite/ite_nand.h"
#endif

static bool             wdEnabled;
static bool             IsChipInfo;
static pthread_mutex_t  mutex_chip_info = PTHREAD_MUTEX_INITIALIZER;

#ifdef CFG_WATCHDOG_IDLETASK
static uint32_t wdLastTick;
#endif

bool
WatchDogGetIsChipInfo (
    void)
{
    bool ret = false;
    (void)pthread_mutex_lock(&mutex_chip_info);
    ret = IsChipInfo;
    (void)pthread_mutex_unlock(&mutex_chip_info);
    return ret;
}

static void
WatchDogRefreshHandler (
    timer_t timerid,
    int     arg)
{
    if (wdEnabled)
    {
        ithWatchDogRestart();
    }

    // ithPrintf("ithWatchDogRestart(),reload=%d,cnt=%d\n", ithWatchDogGetReload(), ithWatchDogGetCounter());

#ifdef  CFG_EXT_WATCHDOG_ENABLE
    if (ithGpioGet(CFG_GPIO_EXT_WATCHDOG))
    {
        ithGpioClear(CFG_GPIO_EXT_WATCHDOG);
    }
    else
    {
        ithGpioSet(CFG_GPIO_EXT_WATCHDOG);
    }
#endif
}

#ifdef CFG_WATCHDOG_IDLETASK

static void
WatchDogIdleRefreshHandler (
    void)
{
    if (itpGetTickDuration(wdLastTick) >= CFG_WATCHDOG_REFRESH_INTERVAL)
    {
        WatchDogRefreshHandler(0, 0);
        wdLastTick = itpGetTickCount();
    }
}

#endif // CFG_WATCHDOG_IDLETASK

#ifdef CFG_WATCHDOG_INTR

static void
WatchDogIntrHandler (
    void * arg)
{
    extern void
    ithDmaWaitAxiSpiIdle (void);
    ithPrintf("Watch Dog Reboot!!!\n");

    #ifdef CFG_DBG_UART0
    while (!ithUartIsTxEmpty(ITH_UART0))
    {
    }
    #elif defined(CFG_DBG_UART1)
    while (!ithUartIsTxEmpty(ITH_UART1))
    {
    }
    #endif

	#ifdef CFG_NAND_ENABLE
    iteNfSwitchToDie0InISR();
	#endif

    ithDelay(10);

    ithDmaWaitAxiSpiIdle();
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_SPI_CLK_REG, 1 << ITH_SPI_RST_BIT, 1 << ITH_SPI_RST_BIT);

    ithWatchDogCtrlEnable(ITH_WD_RESET);
    ithWatchDogSetReload(0);
    ithWatchDogRestart();
    ithWatchDogEnable();
}

#endif // CFG_WATCHDOG_INTR

static void
WatchDogInit (
    void)
{
    bool get_chip_info = ithGetChipInfo();

    (void)pthread_mutex_lock(&mutex_chip_info);
    IsChipInfo = get_chip_info;
    (void)pthread_mutex_unlock(&mutex_chip_info);

    ithWatchDogSetTimeout(CFG_WATCHDOG_TIMEOUT);
    wdEnabled = false;

#ifdef  CFG_EXT_WATCHDOG_ENABLE
    ithGpioSetMode(CFG_GPIO_EXT_WATCHDOG, ITH_GPIO_MODE0);
    ithGpioSetOut(CFG_GPIO_EXT_WATCHDOG);
    ithGpioSet(CFG_GPIO_EXT_WATCHDOG);
    ithGpioEnable(CFG_GPIO_EXT_WATCHDOG);
#endif

#if (defined(CFG_WATCHDOG_IDLETASK)) && (CFG_WATCHDOG_REFRESH_INTERVAL > 0)

    itpRegisterIdleHandler(WatchDogIdleRefreshHandler);

#elif CFG_WATCHDOG_REFRESH_INTERVAL > 0
    timer_t             timer   = 0;
    struct itimerspec   value;
    int                 sec     = CFG_WATCHDOG_REFRESH_INTERVAL / 1000;
    int                 m_sec   = CFG_WATCHDOG_REFRESH_INTERVAL % 1000;

    (void)timer_create(CLOCK_REALTIME, NULL, &timer);
    (void)timer_connect(timer, (VOIDFUNCPTR)&WatchDogRefreshHandler, 0);
    value.it_interval.tv_sec = sec;
    value.it_value.tv_sec = value.it_interval.tv_sec;
    value.it_interval.tv_nsec = m_sec * 1000000;
    value.it_value.tv_nsec = value.it_interval.tv_nsec;
    (void)timer_settime(timer, 0, &value, NULL);
#endif // defined(CFG_WATCHDOG_IDLETASK) && CFG_WATCHDOG_REFRESH_INTERVAL > 0

#ifdef CFG_WATCHDOG_INTR
    ithWatchDogCtrlEnable(ITH_WD_INTR);
    ithIntrRegisterHandlerFiq(ITH_INTR_WD, WatchDogIntrHandler, NULL);
    ithIntrEnableFiq(ITH_INTR_WD);
#endif // CFG_WATCHDOG_INTR
    ithWatchDogCtrlEnable(ITH_WD_RESET);
}

static int
WatchDogIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    int ret = 0;

    switch (request)
    {
        case ITP_IOCTL_INIT:
            WatchDogInit();
            break;

        case ITP_IOCTL_RESET:
            WatchDogRefreshHandler(0, 0);
            break;

        case ITP_IOCTL_ENABLE:
            ithWatchDogRestart();
            ithWatchDogEnable();
            wdEnabled = true;
            break;

        case ITP_IOCTL_DISABLE:
            ithWatchDogDisable();
            wdEnabled = false;
            break;

        default:
            errno = (int)(((uint32_t)ITP_DEVICE_WATCHDOG << ITP_DEVICE_ERRNO_BIT) | (uint32_t)__LINE__);
            ret = -1;
            break;
    }
    return ret;
}

const ITPDevice itpDeviceWatchDog =
{
    ":watchdog",
    &itpOpenDefault,
    &itpCloseDefault,
    &itpReadDefault,
    &itpWriteDefault,
    &itpLseekDefault,
    &WatchDogIoctl,
    NULL
};
