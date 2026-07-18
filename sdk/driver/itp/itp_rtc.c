/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL RTC functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <time.h>
#include <string.h>
#include "itp_cfg.h"

#define RTC_POWER_WORKAROUND    0

#ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
static long	gLastSyncSec    = 0;
static long	gRtcSyncPeriod  = CFG_RTC_SYNC_PERIOD;
#endif
#if (RTC_POWER_WORKAROUND == 1)
static bool g_use_swRTC     = false;
#endif

static int RtcRead(int file, char *ptr, int len, void* info)
{
    if (len > (int)sizeof(uint32_t))
    {
        uint32_t * value = (uint32_t *)ptr;
        
        #ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
        uint32_t us     = 0; 
        uint32_t aRtc   = assistRtcGetTime(&us);
        
        if ( (aRtc - gLastSyncSec) > gRtcSyncPeriod )
        {
            aRtc = itpRtcGetTime(&us);
            if (aRtc != assistRtcGetTime(&us))
            {
                assistRtcSetTime(aRtc, 0);
            }   
            gLastSyncSec = aRtc;
            ITP_LOG_DBG("	[EXT-RTC]Do Sync RTC, syncS==%x, us=%06d\n",gLastSyncSec,us );
        }
        *value = aRtc;
        #else
        *value = itpRtcGetTime(NULL);
        #endif

        return (int)sizeof(uint32_t);
    }
    return 0;
}

static int RtcWrite(int file, char *ptr, int len, void* info)
{
    if (len > (int)sizeof(uint32_t))
    {
        uint32_t * value = (uint32_t *)ptr;
        itpRtcSetTime(*value, 0);
        
        #ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
        {
            assistRtcSetTime(*value, 0); 
        }
        #endif
        	
        return (int)sizeof(uint32_t);
    }
    return 0;
}

static int RtcIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int ret = 0;
#if (RTC_POWER_WORKAROUND == 1)
    void * firstBoot    = NULL;
    void * pattern      = "12345678ABCCDAEB778899AA56789452";
#endif
    
    switch (request)
    {
#if (RTC_POWER_WORKAROUND == 1)
    case ITP_IOCTL_INIT:
        // Use SRAM to record
        firstBoot = (void *) ITH_SRAM_AUDIO_BASE + 0x7D00;

        // check first boot's pattern
        if (memcmp(firstBoot, pattern, 16) == 0) {
            // Use sw RTC
            ithPrintf("[%s] Use Sw-RTC\n", __FUNCTION__);
            itpRtcInit_SW();
            g_use_swRTC = true;            
        }
        else {
            // Use internal RTC
            ithPrintf("[%s] Use internal-RTC\n", __FUNCTION__);
            memcpy(firstBoot, pattern, 16);
            ithFlushDCacheRange(firstBoot, 16);

#ifdef CFG_WATCHDOG_ENABLE
            // set watch dog timeout=100ms
            ithWatchDogSetTimeout(100);
            ithWatchDogRestart();
#endif 
            itpRtcInit();
            g_use_swRTC = false;

#ifdef CFG_WATCHDOG_ENABLE
            // restore watch dog timeout
            ithWatchDogSetTimeout(CFG_WATCHDOG_TIMEOUT * 1000);
            ithWatchDogRestart();
#endif
        }

        // reset firstBoot
        memset(firstBoot, 0, 16);
        ithFlushDCacheRange(firstBoot, 16);

#ifdef  CFG_RTC_REDUCE_IO_ACCESS_ENABLE
        {
            long us = 0;
            assistRtcInit(); 
            assistRtcSetTime(itpRtcGetTime(&us), 0);
        }
#endif
        break;

#else

    case ITP_IOCTL_INIT:        
        itpRtcInit();        
        #ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
        {
            long us;
            assistRtcInit(); 
            assistRtcSetTime(itpRtcGetTime(&us), 0);
        }
        #endif  
        break;
#endif

#if defined(CFG_RTC_CALIBRATE_ENABLE)
    #if (RTC_POWER_WORKAROUND == 1)
    
    case ITP_IOCTL_INIT_TASK:
        if (g_use_swRTC == false) {
            // second_trigger
            RtcSetTriggerType(SEC_TRIGGER);
            itpRtcAdjustInit();
        }
        break;

    case ITP_IOCTL_RTC_ADJUST_DISABLE:
        if (g_use_swRTC == false) {
            RtcAdjustStopTask();
        }
        break;

    case ITP_IOCTL_STANDBY:
        if (g_use_swRTC == false) {
            // change to minute_trigger
            RtcSetTriggerType(MIN_TRIGGER);
            itpRtcAdjustInit();
        }
        break;
        
    #else
    
    case ITP_IOCTL_INIT_TASK:
            // second_trigger
            RtcSetTriggerType(SEC_TRIGGER);
            itpRtcAdjustInit();
        break;

    case ITP_IOCTL_RTC_ADJUST_DISABLE:
            RtcAdjustStopTask();
        break;

    case ITP_IOCTL_STANDBY:
            // change to minute_trigger
            RtcSetTriggerType(MIN_TRIGGER);
            itpRtcAdjustInit();
        break;
        
    #endif
#endif

    case ITP_IOCTL_GET_TIME:
        {
            struct timeval * tv = (struct timeval *)ptr;
            
            #ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
            long aRtc = assistRtcGetTime(&tv->tv_usec);
            if ( (aRtc - gLastSyncSec) > gRtcSyncPeriod )
            {
                long s1     = 0;
                long s2     = aRtc;
                int abs     = 0;
                long rUs    = 0;

                s1 = itpRtcGetTime(&rUs);                
                if ( s1 != s2 )
                {                	
                	if (s1 > s2) {
                        abs = s1 - s2;
                    }
                	else {
                        abs = s2 - s1;
                    }
                
                	if (abs >= 2)
                    {
                    	assistRtcSetTime(s1, 0);
                    	tv->tv_usec = 0;
                	}
                }   
                gLastSyncSec = aRtc;
                ITP_LOG_DBG("	Do Sync Sec with ext-RTC, syncS=%x, us=%06d\n",gLastSyncSec,tv->tv_usec );
            }
            tv->tv_sec = aRtc;
           	#else
#if (RTC_POWER_WORKAROUND == 1)
            if (g_use_swRTC) {
                tv->tv_sec = (time_t) itpRtcGetTime_SW((uint32_t *) &tv->tv_usec);
            }
            else {
                tv->tv_sec = (time_t) itpRtcGetTime((uint32_t *) &tv->tv_usec);
            }
#else
            tv->tv_sec = (time_t) itpRtcGetTime((uint32_t *) &tv->tv_usec);
#endif
            
            #endif
        }
        break;

    case ITP_IOCTL_SET_TIME:
        {
            struct timeval * tv = (struct timeval *)ptr;
#if (RTC_POWER_WORKAROUND == 1)
            if (g_use_swRTC) {
                itpRtcSetTime_SW((uint32_t) tv->tv_sec, (uint32_t) tv->tv_usec);
            }
            else {
                itpRtcSetTime((uint32_t) tv->tv_sec, (uint32_t) tv->tv_usec);
            }
#else
            itpRtcSetTime((uint32_t) tv->tv_sec, (uint32_t) tv->tv_usec);
#endif
            
        	#ifdef	CFG_RTC_REDUCE_IO_ACCESS_ENABLE
      	    assistRtcSetTime(tv->tv_sec, 0);
        	#endif
        }
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

const ITPDevice itpDeviceRtc =
{
    ":rtc",
    NULL,
    NULL,
    &RtcRead,
    &RtcWrite,
    NULL,
    &RtcIoctl,
    NULL
};
