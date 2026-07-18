/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL RTC functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"

static uint32_t divCycle;
static uint32_t lastTime;

static void Reset(void)
{
    ithRtcCtrlEnable(ITH_RTC_RESET);
    ithRtcCtrlDisable(ITH_RTC_RESET);

	/* divCycle is 0x8000 */
	ithWriteRegA(ITH_RTC_BASE + ITH_RTC_DIV_REG, ITH_RTC_DIV_EN_MASK | ITH_RTC_DIV_SRC_MASK | divCycle);

    ithRtcCtrlEnable(ITH_RTC_EN);
}

void ithRtcSetExtSrc(uint32_t source)
{
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_DIV_REG, source, ITH_RTC_DIV_CYCLE_MASK);
}

uint32_t ithRtcGetDivSrc(void)
{
    return (ithReadRegA(ITH_RTC_BASE + ITH_RTC_DIV_REG) & 0x3FFFFFFFU);
}

void ithRtcSetDivSrc(ITHRtcClockSource clkSrc)
{
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_DIV_REG, (uint32_t) clkSrc, (uint32_t) 0x1U << (uint32_t) ITH_RTC_DIV_SRC_BIT);
}

void ithRtcInit(uint32_t extClk)
{
    divCycle = extClk;
    ithRtcCtrlDisable(ITH_RTC_SW_POWEROFF);
}

bool ithRtcEnable(void)
{
    bool firstBoot;
    uint32_t value;

#if (CFG_CHIP_FAMILY == 9860)
    /* AW3 ECO bit */
    ithSetRegBitA(ITH_RTC_BASE + ITH_RTC_CR_REG, 28);
#endif

    value = ithReadRegA(ITH_RTC_BASE + ITH_RTC_DIV_REG);

    if (((value & ITH_RTC_DIV_EN_MASK) == 0U) || 
        ((value & ITH_RTC_DIV_CYCLE_MASK) >= (divCycle + 10U)) || 
        ((value & ITH_RTC_DIV_CYCLE_MASK) <= (divCycle - 10U)))
    {
        Reset();
        firstBoot = true;
    }
    else
    {
        ithRtcCtrlEnable(ITH_RTC_EN);
        firstBoot = false;
    }
    return firstBoot;
}

uint32_t ithRtcGetTime(void)
{
    uint32_t day, hour, min, sec, sec2;

    if ((ithReadRegA(ITH_RTC_BASE + ITH_RTC_CR_REG) & ((uint32_t) 0x1U << (uint32_t) ITH_RTC_COUNTER_LOAD)) != 0U) {
        return lastTime;
    }

    do
    {
        sec     = ithReadRegA(ITH_RTC_BASE + ITH_RTC_SEC_REG) & ITH_RTC_SEC_MASK;
        day     = ithReadRegA(ITH_RTC_BASE + ITH_RTC_DAY_REG) & ITH_RTC_DAY_MASK;
        hour    = ithReadRegA(ITH_RTC_BASE + ITH_RTC_HOUR_REG) & ITH_RTC_HOUR_MASK;
        min     = ithReadRegA(ITH_RTC_BASE + ITH_RTC_MIN_REG) & ITH_RTC_MIN_MASK;
        sec2    = ithReadRegA(ITH_RTC_BASE + ITH_RTC_SEC_REG) & ITH_RTC_SEC_MASK;
    } while (sec != sec2);

    return (day * 24U * 60U * 60U) + (hour * 60U * 60U) + (min * 60U) + sec;
}

void ithRtcSetTime(uint32_t t)
{
    uint32_t hour, min, sec, wday;

    lastTime = t;

	wday = ((t / 86400U) - 11014U) & 7U;
	
    #if 0
	/* Note week day calc magic number */
	#define LEAPOCH (946684800LL + 86400*(31+29))
	secs = t - LEAPOCH;
	days = secs / 86400;
	wday = (3+days)%7;
    #endif
	

    sec = t % 60U;
    t /= 60U;

    min = t % 60U;
    t /= 60U;

    hour = t % 24U;
    t /= 24U;

	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_WWEEK_REG, wday, ITH_RTC_WWEEK_MASK);
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_WDAY_REG, t, ITH_RTC_WDAY_MASK);
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_WHOUR_REG, hour, ITH_RTC_WHOUR_MASK);
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_WMIN_REG, min, ITH_RTC_WMIN_MASK);
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_WSEC_REG, sec, ITH_RTC_WSEC_MASK);
    ithRtcCtrlEnable(ITH_RTC_COUNTER_LOAD);
}

uint32_t ithRtcGetAlarm(void)
{
	uint32_t day, hour, min, sec, sec2;

	if ((ithReadRegA(ITH_RTC_BASE + ITH_RTC_CR_REG) & ((uint32_t) 0x1U << (uint32_t) ITH_RTC_COUNTER_LOAD)) != 0U) {
		return lastTime;
    }

	do
	{
		sec = ithReadRegA(ITH_RTC_BASE + ITH_RTC_ASEC_REG) & ITH_RTC_ASEC_MASK;
		day = ithReadRegA(ITH_RTC_BASE + ITH_RTC_ADAY_REG) & ITH_RTC_ADAY_MASK;
		hour = ithReadRegA(ITH_RTC_BASE + ITH_RTC_AHOUR_REG) & ITH_RTC_AHOUR_MASK;
		min = ithReadRegA(ITH_RTC_BASE + ITH_RTC_AMIN_REG) & ITH_RTC_AMIN_MASK;
		sec2 = ithReadRegA(ITH_RTC_BASE + ITH_RTC_ASEC_REG) & ITH_RTC_ASEC_MASK;
	} while (sec != sec2);

	return (day * 24U * 60U * 60U) + (hour * 60U * 60U) + (min * 60U) + sec;
}

void ithRtcSetAlarm(uint32_t t)
{
	uint32_t hour, min, sec, wday;

	lastTime = t;

	wday = ((t / 86400U) - 11014U) & 7U;
	
    #if 0
	/* Note week day calc magic number */
	#define LEAPOCH (946684800LL + 86400*(31+29))
	secs = t - LEAPOCH;
	days = secs / 86400;
	wday = (3+days)%7;
    #endif
	

	sec = t % 60U;
	t /= 60U;

	min = t % 60U;
	t /= 60U;

	hour = t % 24U;
	t /= 24U;

	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_AWEEK_REG, wday, ITH_RTC_AWEEK_MASK);
	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_ADAY_REG, t, ITH_RTC_ADAY_MASK);
	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_AHOUR_REG, hour, ITH_RTC_AHOUR_MASK);
	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_AMIN_REG, min, ITH_RTC_AMIN_MASK);
	ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_ASEC_REG, sec, ITH_RTC_ASEC_MASK);
}
