#include <stl_clock.h>
#include <openrtos/FreeRTOS.h>
#include <openrtos/task.h>
#include "ite/itp.h"
#include "ith/ith_timer.h"

typedef enum {
    STL_CLOCK_TEST_STATE_UNINIT = 0,
    STL_CLOCK_TEST_STATE_INITED,
    STL_CLOCK_TEST_STATE_FIRST_IT_TIME_RECORDED,
    STL_CLOCK_TEST_STATE_ERROR,
} STL_CLOCK_TEST_STATE;

// To avoid race conditions, three variables are used
// to record the state of isr and general task individually.
static STL_CLOCK_TEST_STATE test_state_;
static STL_CLOCK_TEST_STATE test_state_slow_timer_isr_;
static STL_CLOCK_TEST_STATE test_state_fast_timer_isr_;
static uint32_t high_speed_tick_count_;

// This interrupt will be triggered every second.
static void rtc_sec_it_handler(void *arg)
{
    ithRtcClearIntr(ITH_RTC_SEC);
    switch (test_state_slow_timer_isr_)
    {
    case STL_CLOCK_TEST_STATE_FIRST_IT_TIME_RECORDED:
        {
            uint32_t duration = high_speed_tick_count_;

            if ((75 < duration) && (duration < 125))
            {
                ithPrintf("o(%u)\n", high_speed_tick_count_);
                high_speed_tick_count_ = 0;
            }
            else
            {
                ithPrintf("x(%u)\n", high_speed_tick_count_);
                test_state_slow_timer_isr_ = STL_CLOCK_TEST_STATE_ERROR;
            }
        }
        break;

    case STL_CLOCK_TEST_STATE_UNINIT:
        if (test_state_ != STL_CLOCK_TEST_STATE_UNINIT)
        {
            high_speed_tick_count_ = 0;
            test_state_slow_timer_isr_ = STL_CLOCK_TEST_STATE_FIRST_IT_TIME_RECORDED;
        }
        break;
    }
}

// This interrupt will be triggered every 10ms.
static void timer_it_handler(void *arg)
{
    uint32_t timer = (uint32_t)arg;
    ithTimerClearIntr(timer);
    high_speed_tick_count_++;
    if (high_speed_tick_count_ > 125)
    {
        test_state_fast_timer_isr_ = STL_CLOCK_TEST_STATE_ERROR;
    }
}

stl_status stl_clock_runtime_test(void)
{
    switch (test_state_)
    {
    case STL_CLOCK_TEST_STATE_INITED:
        if (test_state_slow_timer_isr_ == STL_CLOCK_TEST_STATE_ERROR
         || test_state_fast_timer_isr_ == STL_CLOCK_TEST_STATE_ERROR)
        {
            test_state_ = STL_CLOCK_TEST_STATE_ERROR;
        }
        break;

    case STL_CLOCK_TEST_STATE_UNINIT:
        // Initialize the RTC sec interrupt
        ithIntrRegisterHandlerIrq(ITH_INTR_RTCSEC, rtc_sec_it_handler, NULL);
        ithIntrSetTriggerModeIrq(ITH_INTR_RTCSEC, ITH_INTR_EDGE);
        ithRtcCtrlEnable(ITH_RTC_INTR_SEC);
        ithIntrEnableIrq(ITH_INTR_RTCSEC);

        {
            // Initialize Timer IRQ
            uint32_t timer = ITH_TIMER9;
            uint32_t irqn = ITH_INTR_TIMER1 + timer;
            ithIntrDisableIrq(irqn);
            ithIntrClearIrq(irqn);
            ithTimerReset(timer);

            // register Timer Handler to IRQ
            ithIntrRegisterHandlerIrq(irqn, timer_it_handler, (void *)timer);
            // set Timer IRQ to edge trigger
            ithIntrSetTriggerModeIrq(irqn, ITH_INTR_EDGE);
            // set Timer IRQ to detect rising edge
            ithIntrSetTriggerLevelIrq(irqn, ITH_INTR_HIGH_RISING);

            // Enable Timer IRQ
            ithIntrEnableIrq(irqn);
            ithTimerSetTimeout(timer, 10000);
            ithTimerSetLoad(timer, ithTimerGetCounter(timer));
            ithTimerCtrlEnable(timer, ITH_TIMER_PERIODIC);
            ithTimerEnable(timer);
        }

        test_state_ = STL_CLOCK_TEST_STATE_INITED;
        break;
    }

    return (test_state_ != STL_CLOCK_TEST_STATE_ERROR) ? stl_success : stl_error;
}
