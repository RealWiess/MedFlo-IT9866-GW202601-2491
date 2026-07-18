#ifndef ITH_TIMER_GENERIC_H
#define ITH_TIMER_GENERIC_H

#ifndef ITH_TIMER_H
    #error "Never include this file directly. Use ith/ith_timer.h instead."
#endif

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_timer Timer
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ith/ith_defs.h"
#include "ith/ith_reg.h"

/**
 * Timer control definition.
 */
typedef enum
{
    ITH_TIMER_EN       = 0,     ///< Timer enable
    ITH_TIMER_EXTCLK   = 1,     ///< Timer clock source use external
    ITH_TIMER_UPCOUNT  = 2,     ///< Timer up or down count
    ITH_TIMER_ONESHOT  = 3,     ///< Timer one shot
    ITH_TIMER_PERIODIC = 4,     ///< Timer periodic mode
    ITH_TIMER_PWM      = 5,     ///< PWM on Timer
    ITH_TIMER_EN64     = 6      ///< Merge Timers as 64-bit timer
} ITHTimerCtrl;

/**
 * Resets specified timer.
 *
 * @param timer the timer to reset.
 */
void ithTimerReset(ITHTimer timer);

/**
 * Enables specified timer control.
 *
 * @param timer the timer to enable.
 * @param ctrl the control to enable.
 */
void ithTimerCtrlEnable(ITHTimer timer, ITHTimerCtrl ctrl);

/**
 * Disables specified timer control.
 *
 * @param timer the timer to disable.
 * @param ctrl the control to disable.
 */
void ithTimerCtrlDisable(ITHTimer timer, ITHTimerCtrl ctrl);

/**
 * Enables specified timer.
 *
 * @param timer the timer to enable.
 */
static inline void ithTimerEnable(ITHTimer timer)
{
    ithTimerCtrlEnable(timer, ITH_TIMER_EN);
}

/**
 * Disables specified timer.
 *
 * @param timer the timer to disable.
 */
static inline void ithTimerDisable(ITHTimer timer)
{
    ithTimerCtrlDisable(timer, ITH_TIMER_EN);
}

#define CFG_TIMER_OVERFLOW_ISR

#if defined(CFG_TIMER_OVERFLOW_ISR)
extern uint64_t timerTime[ITH_TIMER_COUNT];
extern uint64_t timerCounter[ITH_TIMER_COUNT];
#endif

/**
 * Gets the counter of specified timer.
 *
 * @param timer the timer to get.
 * @return the timer's counter.
 */
static inline uint64_t ithTimerGetCounter(ITHTimer timer)
{
	return (((uint64_t) ithReadRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_CNT_REG)) + timerCounter[timer]);
}

/**
 * Sets the counter of specified timer.
 *
 * @param timer the timer to set.
 * @param count the count value to set.
 */
static inline void ithTimerSetCounter(ITHTimer timer, uint32_t count)
{
	if (timer == ITH_TIMER8)
	{
		///////reset counter start //work around
		ithWriteRegMaskA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + 0xA8, 0x7 << 28, 0x7 << 28); // set intrMask 28, 29, 30

		ithWriteRegMaskA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + ITH_TIMER1_CR_REG + (timer & 0x7) * 0x4, 0x59, 0xD9); // set bit 0,3,4,6 & clear bit 7

		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0x4, count);
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0xc, count);
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0x8, 0);

		ithWriteRegMaskA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + ITH_TIMER1_CR_REG + (timer & 0x7) * 0x4, 0, 0x59); // clear bit 0,3,4,6

		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0x4, 0);
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0xc, 0);
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + 0x8, 0);

		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + 0xA0, 0x7 << ((timer & 0x7) * 4)); // clear IntrRawState
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + 0xA4, 0x7 << ((timer & 0x7) * 4)); // clear IntrState

		ithWriteRegMaskA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + 0xA8, 0, 0x7 << 28); // clear intrMask 28, 29, 30
	}
	else
		ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_CNT_REG, count);

#if defined(CFG_TIMER_OVERFLOW_ISR)
    timerTime[timer] = 0;
    timerCounter[timer] = 0;
#endif
}

/**
 * Sets the load of specified timer.
 *
 * @param timer the timer to set.
 * @param load the load value to set.
 */
static inline void ithTimerSetLoad(ITHTimer timer, uint32_t load)
{
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_LOAD_REG, load);
}

/**
 * Gets the load value of specified timer.
 *
 * @param timer the timer to get.
 * @return the load value.
 */
static inline uint32_t ithTimerGetLoad(ITHTimer timer)
{
    return ithReadRegA(ITH_TIMER_BASE + timer * 0x10 + ITH_TIMER1_LOAD_REG);
}

#if defined(CFG_TIMER_OVERFLOW_ISR)
void ithTimerIsrEnable(ITHTimer timer);

static inline uint32_t ithTimerGetTime(ITHTimer timer)
{
    return (uint32_t)(((uint64_t)ithReadRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_CNT_REG) * (uint64_t)1000000 / (uint64_t)ithGetBusClock()) + timerTime[timer]);
}

#else
/**
 * Gets the time value of specified timer.
 *
 * @param timer the timer to get.
 * @return the time value (us).
 */
static inline uint32_t ithTimerGetTime(ITHTimer timer)
{
    return (uint64_t)ithReadRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_CNT_REG) * 1000000 / ithGetBusClock();
}

#endif

/**
 * Sets the timeout value of specified timer.
 *
 * @param timer the timer to set.
 * @param us the timeout value (us).
 */
static inline void ithTimerSetTimeout(ITHTimer timer, unsigned int us)
{
    ithTimerSetCounter(timer, (uint64_t)ithGetBusClock() * us / 1000000);
}

/**
 * Sets the match value of specified timer.
 *
 * @param timer the timer to set.
 * @param count the count value.
 */
static inline void ithTimerSetMatch(ITHTimer timer, uint32_t count)
{
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_MATCH1_REG, count);
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_MATCH2_REG, count);
}

/**
 * Sets the PWM match value of specified timer.
 *
 * @param timer the timer to set.
 * @param v1 the count1 value.
 * @param v2 the count2 value.
 */
static inline void ithTimerSetPwmMatch(ITHTimer timer, uint32_t v1, uint32_t v2)
{
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_MATCH1_REG, v1);
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + (timer & 0x7) * 0x10 + ITH_TIMER1_MATCH2_REG, v2);
}

/**
 * Clears interrupt of specified timer.
 *
 * @param timer the timer to clear.
 */
static inline void ithTimerClearIntr(ITHTimer timer)
{
    ithWriteRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + ITH_TIMER_INTRSTATE_REG, 0x7 << ((timer & 0x7) * 4));
}

/**
 * Gets interrupt state of specified timer.
 *
 * @return interrupt state.
 */
static inline uint32_t ithTimerGetIntrState(ITHTimer timer)
{
    return ithReadRegA(ITH_TIMER_BASE + ((timer >> 3) & 0x1) * ITH_TIMER_BASE_1TO2_GAP + ITH_TIMER_INTRSTATE_REG);
}

/**
 * Suspends timer module.
 */
void ithTimerSuspend(void);

/**
 * Resumes from suspend mode for timer module.
 */
void ithTimerResume(void);

#ifdef __cplusplus
}
#endif

#endif // ITH_TIMER_GENERIC_H
/** @} */ // end of ith_timer
/** @} */ // end of ith