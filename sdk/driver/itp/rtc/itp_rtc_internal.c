/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL RTC internal functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <sys/ioctl.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "../itp_cfg.h"

/* NOTICE: (void)printf in interrupt_handler should use UART FIFO mode */
// #define DEBUG_PRINT     (void)printf
#define DEBUG_PRINT(...)

#define TMUS_ENABLE             0
#define RTC_TIMER               (CFG_INTERNAL_RTC_TIMER - 1)
#define RTC_YEAR_2106_TIMESTAMP (0xFFFFFFFFU)
#define RTC_YEAR_2100_TIMESTAMP (0xF485E680U)

#ifdef CFG_RTC_CALIBRATE_ENABLE
    #define ADJUST_TIMER        (CFG_RTC_CALIBRATE_TIMER - 1)
    #define RESET_PERIOD        86400
    /* Active calibrate */
    #define DO_COMPENSATE       0
    /* Just measurement */
    #define JUST_MEASUREMENT    0
    /* Rough calibrate */
    #define LAZY_ADJUST         1
    /* Only Lazy-adjust use. PS: LAZY_LIMIT = 5, it means to measurement 5 seconds for calibrate. */
    #define LAZY_LIMIT          5U
#else
    #define ADJUST_TIMER        0
#endif


#ifdef CFG_RTC_CALIBRATE_ENABLE


static pthread_t        adjustTask;
static uint64_t         gExpectCounter  = 0UL;
static uint64_t         g_old_counter   = 0UL;
static bool             g_first_trigger = true;
static uint32_t         g_odd_count     = 0U;
static double           gRealSource     = (double) CFG_RTC_EXTCLK;
static TriggerType      g_trigger_type  = NO_TRIGGER;
    #if (LAZY_ADJUST == 1)   
    /* Record RTC source */
    typedef struct rtcRecord {
    uint32_t    rtc_source;
    int         count;
    } RtcRecord;

    static uint32_t         g_lazy_range    = 0U;
    static RtcRecord        gSourceRcord[2];  
    #else
    /* LAZY_ADJUST is 0 */
    static pthread_cond_t   adjustCond;
    static pthread_mutex_t  adjustMutex;
    #endif
    #if (DO_COMPENSATE == 1)
    static bool             gCompAdd        = true;
    static uint64_t         gCompCounter    = 0UL;
    #endif
#endif

static void
RtcSecIntrHandler (
    void * arg)
{
#if (TMUS_ENABLE == 0)
    /* reset counter */
    ithTimerSetCounter(RTC_TIMER, 0U);
#endif

#if defined(CFG_RTC_CALIBRATE_ENABLE)
    uint64_t    tmp_counter     = 0UL;
    uint64_t    tmpRealCounter  = 0UL;
    uint64_t    RealCounter     = 0UL;
    double      ExpectSource    = 0.0;
    double      tmpSetSource    = 0.0;
    uint32_t    SetSource       = 0U;
    #if (DO_COMPENSATE == 1)
    uint64_t    SetCounter      = 0UL;
    double      tmpSetCounter   = 0.0;
    #endif

    if (g_trigger_type == SEC_TRIGGER)
    {
        tmp_counter     = ithTimerGetCounter(ADJUST_TIMER);
        tmpRealCounter  = tmp_counter - g_old_counter;
        g_old_counter   = tmp_counter;

        if (tmpRealCounter > 0U)
        {
            if (g_first_trigger == false)
            {
                if ((g_odd_count % 2U) != 0U)
                {
    #if (DO_COMPENSATE == 1)
                    /* add or sub for the last time of counter */
                    if (gCompAdd)
                    {
                        RealCounter = tmpRealCounter + gCompCounter;
                    }
                    else
                    {
                        RealCounter = tmpRealCounter - gCompCounter;
                    }
    #else
                    RealCounter = tmpRealCounter;
    #endif

                    /* calculate ExpectSource */
    #if (JUST_MEASUREMENT == 1)
                    ExpectSource    = (double)32768 * ((double)gExpectCounter / (double)RealCounter);
                    tmpSetSource    = round(ExpectSource);
    #elif (LAZY_ADJUST == 1)
                    ExpectSource    = (double)gRealSource * ((double)gExpectCounter / (double)RealCounter);

                    /* rule of thumb: MAYBE it depends by case. */
                    if ((ExpectSource >= 32768.0) && (ExpectSource < 32769.0))
                    {
                        tmpSetSource = floor(ExpectSource);
                    }
                    else if (ExpectSource >= 32772.0)
                    {
                        /* tmpSetSource = round(ExpectSource) + 1; */
                        tmpSetSource = ceil(ExpectSource);
                    }
                    else
                    {
                        tmpSetSource = round(ExpectSource);
                    }
    #else
                    ExpectSource    = (double)gRealSource * ((double)gExpectCounter / (double)RealCounter);
                    tmpSetSource    = floor(ExpectSource);
    #endif
                    SetSource       = (uint32_t) tmpSetSource;

    #if (DO_COMPENSATE == 1)
                    /* calculate compensated counter for the next real counter */
                    tmpSetCounter   = (double)gExpectCounter * ((double)gRealSource / (double)SetSource);
                    SetCounter      = (uint64_t) floor(tmpSetCounter);

                    if (RealCounter > SetCounter)
                    {
                        /* add compensated counter */
                        gCompCounter    = RealCounter - SetCounter;
                        gCompAdd        = true;
                    }
                    else
                    {
                        /* sub compensated counter */
                        gCompCounter    = SetCounter - RealCounter;
                        gCompAdd        = false;
                    }
    #endif

    #if (JUST_MEASUREMENT == 0) && (LAZY_ADJUST == 0)
                    /* Set RTC Source after adjustment */
                    ithRtcSetExtSrc(SetSource);
                    gRealSource = (double) SetSource;
    #endif

    #if (LAZY_ADJUST == 1)
                    int     anchor  = 0;
                    int     i       = 0;
                    bool    found   = false;

                    for (i = 0; i < 2; i++)
                    {
                        if (gSourceRcord[i].rtc_source == SetSource)
                        {
                            found   = true;
                            anchor  = i;
                        }
                    }

                    if (found)
                    {
                        gSourceRcord[anchor].count = gSourceRcord[anchor].count + 1;
                    }
                    else
                    {
                        if (gSourceRcord[0].rtc_source == 0U)
                        {
                            gSourceRcord[0].rtc_source  = SetSource;
                            gSourceRcord[0].count       = gSourceRcord[0].count + 1;
                        }
                        else if (gSourceRcord[1].rtc_source == 0U)
                        {
                            gSourceRcord[1].rtc_source  = SetSource;
                            gSourceRcord[1].count       = gSourceRcord[1].count + 1;
                        }
                        else
                        {
                            DEBUG_PRINT("[%s] ERROR: Source Record(%d). [0]=%d [1]=%d\n", __FUNCTION__, SetSource, gSourceRcord[0].rtc_source, gSourceRcord[1].rtc_source);
                        }
                    }

                    g_lazy_range++;
    #endif
                }
                else
                {
                    /* Do Nothing */
                }

    #if (JUST_MEASUREMENT == 1) || (LAZY_ADJUST == 1)
                DEBUG_PRINT("[%s] SetSource=%d ExpectSource=%lf RealCounter=%d\n", __FUNCTION__, SetSource, ExpectSource, (uint32_t)tmpRealCounter);
                g_odd_count = 1U; /* always odd */
    #else
                DEBUG_PRINT("[%s][%d] Source=%d RealCounter=%d CompAdd=%d CompCounter=%d\n", __FUNCTION__, g_odd_count, SetSource, (uint32_t)tmpRealCounter, gCompAdd, (uint32_t)gCompCounter);
                g_odd_count++;
    #endif
            }
            else
            {
                g_first_trigger = false;
                g_odd_count     = 0U;                
    #if (DO_COMPENSATE == 1)
                gCompAdd        = true;
                gCompCounter    = 0UL;
    #endif
                DEBUG_PRINT("[%s] first trigger.\n", __FUNCTION__);
            }
        }
        else
        {
            g_first_trigger = true;
            g_odd_count     = 0U;            
    #if (DO_COMPENSATE == 1)
            gCompAdd        = true;
            gCompCounter    = 0UL;
    #endif
            DEBUG_PRINT("[%s] timer counter is zero\n", __FUNCTION__);
        }
    }
#endif

    ithRtcClearIntr(ITH_RTC_SEC);
}

#if defined(CFG_RTC_CALIBRATE_ENABLE)

static void
RtcMinIntrHandler (
    void * arg)
{
    uint64_t    tmp_counter     = 0UL;
    uint64_t    tmpRealCounter  = 0UL;
    uint64_t    RealCounter     = 0UL;
    double      ExpectSource    = 0.0;
    double      tmpSetSource    = 0.0;
    uint32_t    SetSource       = 0U;
    uint32_t    IntrStReg       = 0U;
    #if (DO_COMPENSATE == 1)
    uint64_t    SetCounter      = 0UL;
    double      tmpSetCounter   = 0.0;
    #endif

    /* Get RTC interrupt status */
    IntrStReg = ithRtcGetIntrState();

    if ((IntrStReg & 0x2U) != 0x0U)
    {
        tmp_counter     = ithTimerGetCounter(ADJUST_TIMER);
        tmpRealCounter  = tmp_counter - g_old_counter;
        g_old_counter   = tmp_counter;

        if (tmpRealCounter > 0UL)
        {
            if (g_first_trigger == false)
            {
    #if (DO_COMPENSATE == 1)
                /* add or sub for the last time of counter */
                if (gCompAdd)
                {
                    RealCounter = tmpRealCounter + gCompCounter;
                }
                else
                {
                    RealCounter = tmpRealCounter - gCompCounter;
                }
    #else
                RealCounter = tmpRealCounter;
    #endif

                /* calculate ExpectSource */
    #if (JUST_MEASUREMENT == 1)
                ExpectSource    = (double)32768 * ((double)gExpectCounter / (double)RealCounter);
    #else
                ExpectSource    = (double)gRealSource * ((double)gExpectCounter / (double)RealCounter);
    #endif
                tmpSetSource    = floor(ExpectSource);
                SetSource       = (uint32_t) tmpSetSource;

    #if (DO_COMPENSATE == 1)
                /* calculate compensated counter for the next real counter */
                tmpSetCounter   = (double)gExpectCounter * ((double)gRealSource / (double)SetSource);
                SetCounter      = (uint64_t) floor(tmpSetCounter);

                if (RealCounter > SetCounter)
                {
                    /* add compensated counter */
                    gCompCounter    = RealCounter - SetCounter;
                    gCompAdd        = true;
                }
                else
                {
                    /* sub compensated counter */
                    gCompCounter    = SetCounter - RealCounter;
                    gCompAdd        = false;
                }
    #endif

    #if (JUST_MEASUREMENT == 1)
                DEBUG_PRINT("[%s] SetSource=%d ExpectSource=%lf RealCounter=%d\n", __FUNCTION__, SetSource, ExpectSource, (uint32_t)tmpRealCounter);
    #else
                /* Set RTC Source after adjustment */
                ithRtcSetExtSrc(SetSource);
                gRealSource = (double) SetSource;
                DEBUG_PRINT("[%s][%d] MIN Source=%d RealCounter=%llu CompAdd=%d CompCounter=%llu\n", __FUNCTION__, g_odd_count, SetSource, tmpRealCounter, gCompAdd, gCompCounter);
    #endif
                g_odd_count++;
            }
            else
            {
                g_first_trigger = false;               
                g_odd_count     = 0U;
    #if (DO_COMPENSATE == 1)
                gCompAdd        = true;
                gCompCounter    = 0UL;
    #endif
                DEBUG_PRINT("[%s] first trigger.\n", __FUNCTION__);
            }
        }
        else
        {
            g_first_trigger = true;            
            g_odd_count     = 0U;
    #if (DO_COMPENSATE == 1)
            gCompAdd        = true;
            gCompCounter    = 0UL;
    #endif
            DEBUG_PRINT("[%s] timer counter is zero\n", __FUNCTION__);
        }

        ithRtcClearIntr(ITH_RTC_MIN);
    }
}
#endif

#if defined(CFG_RTC_CALIBRATE_ENABLE) && (LAZY_ADJUST == 0)

static void *
RtcAdjustTask (
    void * arg)
{
    int             ret = 0;
    time_t          T;
    struct timespec t;

    /* delay a little */
    (void)sleep(1);

    /* Get Expect Counter according to BusClk and the trigger period(second/minute) */
    if (g_trigger_type == SEC_TRIGGER)
    {
        /* second trigger */
        gExpectCounter = (uint64_t)ithGetBusClock();
    }
    else if (g_trigger_type == MIN_TRIGGER)
    {
        /* minute trigger */
        gExpectCounter = (uint64_t)ithGetBusClock() * 60UL;
    }
    else
    {
        (void)printf("[%s] ERROR: trigger type(%d).\n", __FUNCTION__, g_trigger_type);
    }

    DEBUG_PRINT("[%s] start. RtcSource=%lf ExpectCounter=%llu Trigger_type=%d\n", __FUNCTION__, gRealSource, gExpectCounter, trigger_type);

    for (;;)
    {
        /* Avoid timer counter > 0 between sec_interrupt */
        g_first_trigger = true;
        /* Reset Timer */
        ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
        ithTimerReset(ADJUST_TIMER);                            /* reset timer */
        ithTimerCtrlEnable(ADJUST_TIMER, ITH_TIMER_UPCOUNT);    /* up count the counter */
        ithTimerSetCounter(ADJUST_TIMER, 0x0);                  /* init counter to 0 */
        ithTimerSetLoad(ADJUST_TIMER, 0x0);                     /* set reload */

        /* minute interrupt */
        if (g_trigger_type == MIN_TRIGGER)
        {
            /* Clear interrupt */
            ithRtcClearIntr(ITH_RTC_MIN);

            /* init rtc min interrupt */
            ithRtcCtrlEnable(ITH_RTC_INTR_MIN);
            ithIntrRegisterHandlerIrq(ITH_INTR_RTC, RtcMinIntrHandler, NULL);
            ithIntrSetTriggerModeIrq(ITH_INTR_RTC, ITH_INTR_EDGE);
            ithIntrEnableIrq(ITH_INTR_RTC);
        }

        /* timer start */
        ithTimerIsrEnable(ADJUST_TIMER);
        ithTimerEnable(ADJUST_TIMER);

        /* Reset timer for timer counter overflow */
        (void)pthread_mutex_lock(&adjustMutex);
        (void)time(&T);
        t.tv_sec    = T + RESET_PERIOD;
        t.tv_nsec   = 0;
        do {
            ret = pthread_cond_timedwait(&adjustCond, &adjustMutex, &t);
        } while (ret != 0);
        (void)pthread_mutex_unlock(&adjustMutex);

        /* check if thread stop because of signal? */
        if (ret == 0)
        {
            /* stop thread */
            break;
        }
        DEBUG_PRINT("[%s] Reset timer. RTC=%ld\n", __FUNCTION__, ithRtcGetTime());
    }

    DEBUG_PRINT("[%s] end\n", __FUNCTION__);

    return NULL;
}
#endif

#if defined(CFG_RTC_CALIBRATE_ENABLE) && (LAZY_ADJUST == 1)

static void *
RtcLazyAdjustTask (
    void * arg)
{
    int i = 0;

    DEBUG_PRINT("[%s] start\n", __FUNCTION__);

    /* delay a little */
    (void)sleep(1);

    /* init SourceRcord */
    for (i = 0; i < 2; i++)
    {
        gSourceRcord[i].rtc_source  = 0;
        gSourceRcord[i].count       = 0;
    }

    /* Get Expect Counter according to BusClk and the trigger period(second) */
    gExpectCounter = (uint64_t)ithGetBusClock();

    DEBUG_PRINT("[%s] start. RtcSource=%lf ExpectCounter=%llu Trigger_type=%d\n", __FUNCTION__, gRealSource, gExpectCounter, trigger_type);

    /* Avoid timer counter > 0 between sec_interrupt */
    g_first_trigger = true;
    /* Reset Timer */
    ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
    ithTimerReset(ADJUST_TIMER);                            /* reset timer */
    ithTimerCtrlEnable(ADJUST_TIMER, ITH_TIMER_UPCOUNT);    /* up count the counter */
    ithTimerSetCounter(ADJUST_TIMER, 0x0);                  /* init counter to 0 */
    ithTimerSetLoad(ADJUST_TIMER, 0x0);                     /* set reload */

    /* timer start */
    ithTimerIsrEnable(ADJUST_TIMER);
    ithTimerEnable(ADJUST_TIMER);

    for (;;)
    {
        if (g_lazy_range >= LAZY_LIMIT)
        {
            /* stop lazy-measurement in secInterrupt */
            g_trigger_type = NO_TRIGGER;

            for (i = 0; i < 2; i++)
            {
                DEBUG_PRINT("[%s] [%d].Source=%d Count=%d\n", __FUNCTION__, i, gSourceRcord[i].rtc_source, gSourceRcord[i].count);
            }

            /* set rtc source */
            if (gSourceRcord[0].count > gSourceRcord[1].count)
            {
                ITP_LOG_INFO("[%s] Set RTC Source from %.1lf to %" PRIu32 "\n", __FUNCTION__, gRealSource, gSourceRcord[0].rtc_source);
                ithRtcSetExtSrc(gSourceRcord[0].rtc_source);
            }
            else
            {
                ITP_LOG_INFO("[%s] Set RTC Source from %.1lf to %" PRIu32 "\n", __FUNCTION__, gRealSource, gSourceRcord[1].rtc_source);
                ithRtcSetExtSrc(gSourceRcord[1].rtc_source);
            }

            break;
        }

        (void)sleep(1);
    }

    /* disable timer */
    ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
    ithTimerReset(ADJUST_TIMER);

    DEBUG_PRINT("[%s] end\n", __FUNCTION__);
    return NULL;
}
#endif

#if defined(CFG_RTC_CALIBRATE_ENABLE)
void
RtcAdjustStopTask (
    void)
{
    if (g_trigger_type == MIN_TRIGGER)
    {
        /* stop minute interrupt */
        ithRtcCtrlDisable(ITH_RTC_INTR_MIN);
        ithIntrDisableIrq(ITH_INTR_RTC);
        ithRtcClearIntr(ITH_RTC_MIN);
    }

    /* disable timer */
    ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
    ithTimerReset(ADJUST_TIMER);
    g_first_trigger = true;

    #if (LAZY_ADJUST == 0)
    /* Stop RtcAdjust task */
    (void)pthread_mutex_lock(&adjustMutex);
    (void)pthread_cond_signal(&adjustCond);
    (void)pthread_mutex_unlock(&adjustMutex);

    /* joint adjustTask */
    if (pthread_join(adjustTask, NULL) != 0)
    {
        (void)printf("[%s] RtcAdjustTask stop fail\n", __FUNCTION__);
    }

    /* Destory cond */
    (void)pthread_cond_destroy(&adjustCond);

    /* Destory mutex */
    (void)pthread_mutex_destroy(&adjustMutex);
    #endif

    /* Reset default varible */
    gExpectCounter  = 0;  
    g_old_counter   = 0;
    g_odd_count     = 0U;
    #if (DO_COMPENSATE == 1)
    gCompCounter    = 0;
    gCompAdd        = true;
    #endif   
}

void
RtcSetTriggerType (
    TriggerType type)
{
    /* 1: second_trigger; 2: minute_trigger */
    g_trigger_type = type;
}

void
itpRtcAdjustInit (
    void)
{
    /* init defalut varible */
    gExpectCounter  = 0;    
    g_old_counter   = 0;
    g_odd_count     = 0;    
    #if (DO_COMPENSATE == 1)
    gCompAdd        = true;
    gCompCounter    = 0;
    #endif
    /* Get the current RTC source */
    gRealSource     = (double) ithRtcGetDivSrc();

    #if (LAZY_ADJUST == 1)
    /* Create a lazy task */
    g_first_trigger = true;
    (void)pthread_create(&adjustTask, NULL, RtcLazyAdjustTask, NULL);
    #else
    /* Init adjustTask cond */
    (void)pthread_cond_init(&adjustCond, NULL);

    /* Init adjustTask Mutex */
    (void)pthread_mutex_init(&adjustMutex, NULL);

    /* Create a task */
    g_first_trigger = true;
    (void)pthread_create(&adjustTask, NULL, RtcAdjustTask, NULL);
    #endif
}
#endif

#if (TMUS_ENABLE == 1)
static uint32_t
RtcGetUs (
    void)
{
    uint32_t usec = 0x0;

    usec = ithReadRegA(ITH_TIMER_BASE + ITH_TIMER_TMUS_COUNTER_REG) & 0x0FFFFFFFU;

    return usec;
}
#endif

void
itpRtcInit (
    void)
{
#if 0
    {
        // check RTC clk is live
        uint32_t    wd_counter  = 0U;
        uint32_t    timeout     = 100000000U;

        ithSetRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 4U);
        ithSetRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 0U);

        wd_counter = ithWatchDogGetCounter();

        while (wd_counter == ithWatchDogGetCounter())
        {
            if (timeout <= 0U)
            {
                ithPrintf("RTC's power is dead\n");
                timeout = 100000000U;
            }
            timeout--;
        }

        ithClearRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 0U);
        ithClearRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 4U);
    }
#endif

    ithRtcInit(CFG_RTC_EXTCLK);
    if (ithRtcEnable())
    {
        ITP_LOG_INFO("First time boot\n");
        ithRtcSetTime(CFG_RTC_DEFAULT_TIMESTAMP);
    }

#if (TMUS_ENABLE == 1)
    /* init TmUs counter to calc usec of gettimeofday() */
    ithWriteRegA(ITH_TIMER_BASE + ITH_TIMER_TMUS_EN_REG, (ithGetBusClock() / 1000000U) - 1U);
    ithSetRegBitA(ITH_TIMER_BASE + ITH_TIMER_TMUS_EN_REG, 31);
#else
    /* init timer to calc usec of gettimeofday() */
    ithTimerReset(RTC_TIMER);
    ithTimerCtrlEnable(RTC_TIMER, ITH_TIMER_UPCOUNT);
    ithTimerSetCounter(RTC_TIMER, 0U);
    ithTimerEnable(RTC_TIMER);
#endif

    /* init rtc sec interrupt */
    ithRtcCtrlEnable(ITH_RTC_INTR_SEC);
    ithIntrRegisterHandlerIrq(ITH_INTR_RTCSEC, RtcSecIntrHandler, NULL);
    ithIntrSetTriggerModeIrq(ITH_INTR_RTCSEC, ITH_INTR_EDGE);
    ithIntrEnableIrq(ITH_INTR_RTCSEC);

    DEBUG_PRINT("[%s] Use internal RTC, Source=%u\n", __FUNCTION__, ithRtcGetDivSrc());
}

uint32_t
itpRtcGetTime (
    uint32_t * usec)
{
    uint32_t    sec1    = 0;
    uint32_t    sec2    = 0;
    do
    {
        sec1 = ithRtcGetTime();
        if (usec != NULL)
        {
#if (TMUS_ENABLE == 1)
            *usec   = RtcGetUs();
#else
            *usec   = ithTimerGetTime(RTC_TIMER);
#endif
        }

        sec2 = ithRtcGetTime();
    } while (sec1 != sec2);

    if ((sec1 < (uint32_t) CFG_RTC_DEFAULT_TIMESTAMP) ||
        (sec1 > RTC_YEAR_2100_TIMESTAMP))
    {
        /* SW handle internal RTC low bettery power issue */
        /* report default timestemp when RTC < default or RTC > year 2100 */
        uint32_t    stemp_def   = (uint32_t) CFG_RTC_DEFAULT_TIMESTAMP;
        uint32_t    stemp_2100  = RTC_YEAR_2100_TIMESTAMP;

        ITP_LOG_ERR("[RTC] get time ERROR(ori=%x, default=%x, 2100=%x):[#%d]\n", sec1, stemp_def, stemp_2100, __LINE__);
        sec1 = CFG_RTC_DEFAULT_TIMESTAMP;

        /* Reset RTC timestamp */
        ithRtcSetTime(CFG_RTC_DEFAULT_TIMESTAMP);
    }

    return sec1;
}

void
itpRtcSetTime (
    uint32_t    sec,
    uint32_t    usec)
{
    ITP_LOG_DBG("[RTC] set time: sec=%x, usec=%x\n", sec, usec);

    if ((sec < (uint32_t) CFG_RTC_DEFAULT_TIMESTAMP) ||
        (sec > RTC_YEAR_2100_TIMESTAMP))
    {
        /* SW handle internal RTC low bettery power issue */
        /* set default timestemp when sec < default or sec > year 2100 */
        ITP_LOG_ERR("[RTC] set time ERROR(ori=%x, default=%x):[#%d]\n", sec, CFG_RTC_DEFAULT_TIMESTAMP, __LINE__);
        ithRtcSetTime(((uint32_t) CFG_RTC_DEFAULT_TIMESTAMP) + (usec / 1000000U));
    }
    else
    {
        ithRtcSetTime(sec + (usec / 1000000U));
    }

#if defined(CFG_RTC_CALIBRATE_ENABLE)

    if (g_trigger_type == MIN_TRIGGER)
    {
        /* stop minute interrupt */
        ithRtcCtrlDisable(ITH_RTC_INTR_MIN);
        ithIntrDisableIrq(ITH_INTR_RTC);
        ithRtcClearIntr(ITH_RTC_MIN);
    }

    /* disable timer */
    ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
    ithTimerReset(ADJUST_TIMER);
    g_first_trigger = true;

    /* Reset default varible */
    #if (DO_COMPENSATE == 1)
    gCompCounter    = 0;
    gCompAdd        = true;
    #endif
    g_old_counter   = 0;
    g_odd_count     = 0U;   

    /* Reset Timer */
    ithTimerCtrlDisable(ADJUST_TIMER, ITH_TIMER_EN);
    ithTimerReset(ADJUST_TIMER);                            /* reset timer */
    ithTimerCtrlEnable(ADJUST_TIMER, ITH_TIMER_UPCOUNT);    /* up count the counter */
    ithTimerSetCounter(ADJUST_TIMER, 0U);                  /* init counter to 0 */
    ithTimerSetLoad(ADJUST_TIMER, 0U);                     /* set reload */

    if (g_trigger_type == MIN_TRIGGER)
    {
        /* init rtc min interrupt */
        ithRtcCtrlEnable(ITH_RTC_INTR_MIN);
        ithIntrRegisterHandlerIrq(ITH_INTR_RTC, RtcMinIntrHandler, NULL);
        ithIntrSetTriggerModeIrq(ITH_INTR_RTC, ITH_INTR_EDGE);
        ithIntrEnableIrq(ITH_INTR_RTC);
    }

    #if (LAZY_ADJUST == 0)
    /* timer start */
    ithTimerIsrEnable(ADJUST_TIMER);
    ithTimerEnable(ADJUST_TIMER);
    #endif

#endif

    DEBUG_PRINT("[%s] sec=%d\n", __FUNCTION__, ithRtcGetTime());
}
