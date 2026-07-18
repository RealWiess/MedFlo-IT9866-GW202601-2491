/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL RTC software functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <time.h>
#include "../itp_cfg.h"

static uint32_t g_rtcSec       = 0U;
static uint32_t g_rtcMsec      = 0U;
static uint32_t g_rtcLastMS    = 0U;

static void RtcHandler(timer_t timerid, int arg)
{
    uint32_t ms     = 0;
    uint32_t diff   = 0;
    uint64_t last   = 0;
    uint64_t now    = 0;

    ithEnterCritical();

    ms = itpGetTickCount();
    if (ms >= g_rtcLastMS) {
        diff = ms - g_rtcLastMS;
    }
    else {
        diff = ms + (0xFFFFFFFFU - g_rtcLastMS); // overflow
    }
    
    last = ((uint64_t)g_rtcSec * 1000UL) + g_rtcMsec;
    now = last + (uint64_t) diff;
    
    g_rtcLastMS   = ms;
    g_rtcSec      = (uint32_t) (now / 1000UL);
    g_rtcMsec     = (uint32_t) (now % 1000UL);
    
    ithExitCritical();
}

void itpRtcInit_SW(void)
{
    timer_t timerId;
    struct itimerspec value;

    value.it_value.tv_sec       = 60;
    value.it_value.tv_nsec      = 0;
    value.it_interval.tv_sec    = value.it_value.tv_sec;
    value.it_interval.tv_nsec   = value.it_value.tv_nsec;

    (void)timer_create(CLOCK_REALTIME, NULL, &timerId);
    (void)timer_connect(timerId, &RtcHandler, 0);
    
    g_rtcLastMS = itpGetTickCount();
    g_rtcMsec = (g_rtcLastMS % 1000U) * 1000U;
    g_rtcSec  = (uint32_t) CFG_RTC_DEFAULT_TIMESTAMP + (g_rtcLastMS / 1000U);

    (void)timer_settime(timerId, 0, &value, NULL);
}

uint32_t itpRtcGetTime_SW(uint32_t* usec)
{
    uint32_t ms     = 0;
    uint32_t diff   = 0;
    uint64_t last   = 0;
    uint64_t now    = 0;

    ithEnterCritical();

    ms = itpGetTickCount();
    if (ms >= g_rtcLastMS) {
        diff = ms - g_rtcLastMS;
    }
    else {
        diff = ms + (0xFFFFFFFFU - g_rtcLastMS); // overflow
    }

    last = ((uint64_t)g_rtcSec * 1000UL) + (uint64_t)g_rtcMsec;
    now = last + diff;
    
    g_rtcLastMS   = ms;
    g_rtcSec      = (uint32_t) (now / 1000UL);
    g_rtcMsec     = (uint32_t) (now % 1000UL);
    
    if (usec != NULL) {
        *usec = g_rtcMsec * 1000U;
    }

    ithExitCritical();

    return g_rtcSec;
}

void itpRtcSetTime_SW(uint32_t sec, uint32_t usec)
{
    ithEnterCritical();
    
    g_rtcLastMS   = itpGetTickCount();
    g_rtcSec      = sec;
    g_rtcMsec     = usec / 1000U;

    ithExitCritical();
}
