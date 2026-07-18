/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL TP functions.
 *
 * @author Joseph
 * @version 1.0
 */
#include <stdlib.h>
#include <pthread.h>

#include <errno.h>
#include <time.h>
#include "itp_cfg.h"

#if (CFG_GPIO_TOUCH_RESET < 200) && (CFG_GPIO_TOUCH_RESET > 0)
    #define TP_RESET_PIN    CFG_GPIO_TOUCH_RESET
#else
    #define TP_RESET_PIN    (-1)
#endif

volatile int gTpPowerOnSeqDone = 0;

extern void (*tp_DoPowerOnSeq_callback)(void);

void *
tp_DoPowerOnSeqThread (
    void * arg)
{
    // (void)printf("   tp_thread_entry:\n");
#ifdef CFG_ITP_TP_INIT_ENABLE
    tp_DoPowerOnSeq_callback();
#endif

    ithEnterCritical();
    gTpPowerOnSeqDone = 1;
    ithExitCritical();

    // (void)printf("   tp_thread_exit:\n");

    return NULL;
}

static void
TpInit (
    void)
{
    char            rstPin = (char)TP_RESET_PIN;
    int             i;
    int             res;
    pthread_t       task;
    pthread_attr_t  attr;

    // do power reset flow of touch panel
    (void)printf("Do TP Power-On flow\n");
    if ((rstPin == -1) || (rstPin < 4))
    {
        (void)printf("tp power-on sequence: skip it, because rstPin < 4, p = %d !!\n", rstPin);
        return;
    }

    // create thread
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    res = pthread_create(&task, &attr, tp_DoPowerOnSeqThread, NULL);
    if (res)
    {
        (void)printf("[TpInit]: ERROR, create tp_DoPowerOnSeqThread() thread fail! res=%d\n", res);
    }
    return;
}

static void
TpChkDeviceReady (
    void)
{
    (void)printf("check TP ready\n");
}

static int
TpOpen (
    const char  * name,
    int         flags,
    int         mode,
    void        * info)
{
    int index = atoi(name);
    return index;
}

static int
TpClose (
    int     file,
    void    * info)
{
    ithEnterCritical();
    gTpPowerOnSeqDone = 0;
    ithExitCritical();
    return 0;
}

static int
TpIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    switch (request)
    {
        case ITP_IOCTL_INIT:
            (void)printf("itp_tp_ioctl: init\n");
            TpInit();
            break;

        case ITP_IOCTL_IS_DEVICE_READY:
        {
            int tmp = gTpPowerOnSeqDone;

            if (tmp)
            {
                return (int)(0);        // 0: ready
            }
            else
            {
                return (int)(1);        // 1: BUSY
            }
            break;
        }

        default:
            errno = (ITP_DEVICE_LED << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceTp =
{
    ":tp",
    TpOpen,
    TpClose,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    TpIoctl,
    NULL
};
