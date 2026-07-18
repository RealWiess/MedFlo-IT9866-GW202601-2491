///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <Utility.c>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
// ******************************************/
#include <sys/time.h>
#include "stdio.h"
#include "hdmirx/it66021/config.h"
#include "hdmirx/it66021/IO.h"
#include "hdmirx/it66021/Utility.h"

unsigned int prevTickCount = 0;

unsigned int
CalTimer (
    void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
    {
        (void)printf("gettimeofday failed!\n");
    }
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

unsigned int
CalTimerDuration (
    unsigned int clock)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
    {
        (void)printf("gettimeofday failed!\n");
    }
    return tv.tv_sec * 1000 + tv.tv_usec / 1000 - clock;
}

int
TimeOutCheck (
    unsigned int    timer,
    unsigned int    x)
{
    if (CalTimerDuration(timer) >= x)
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
IsTimeOut (
    unsigned int x)
{
    if (CalTimerDuration(prevTickCount) >= x)
    {
        prevTickCount = CalTimer();
        return TRUE;
    }
    return FALSE;
}

unsigned int
GetCurrentVirtualTime ()
{
    return CalTimer();
}

void
delay1ms (
    unsigned short ms)
{
    usleep(ms * 1000);
}
