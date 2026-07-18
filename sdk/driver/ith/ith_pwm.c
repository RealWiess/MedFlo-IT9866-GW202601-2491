/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL PWM functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"
#include <pthread.h>

static uint32_t blCounts[ITH_PWM_COUNT], blMatchs[ITH_PWM_COUNT];

void ithPwmInit(ITHPwm pwm, unsigned int freq, unsigned int duty)
{
    if (duty != 100)
    {
        blCounts[pwm] = (duty) ? (ithGetBusClock() / freq) - 1 : duty;
        blMatchs[pwm] = (duty) ? (uint64_t)blCounts[pwm] * (100 - duty % 100) / 100 : -1;
    }
    else
    {
        blCounts[pwm] = 0;
        blMatchs[pwm] = 0;
    }
}

static void ithPwmSetGpioMode(ITHPwm pwm, unsigned int pin)
{
#if (CFG_CHIP_FAMILY == 9860)
        if (pin >= 21 && pin <= 24)
        {
            (void)ithPrintf("Hit fully mux special case, pin: %d change to %d\n", pin, pin-21);
            pin -= 21;
        }

        if (pin >= 68 && pin <= 77) // using MIPI GPIO
            ithWriteRegMaskA(ITH_MIPI_DPHY_BASE, 0x2 << 22, 0x3 << 22); // set MIPI DPHY R0 to TTL
#endif

    switch (pwm) {
        case ITH_PWM1:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, pin << ITH_GPIO_PWM_LOW_BIT, ITH_GPIO_PWM_LOW_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM2:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, pin << ITH_GPIO_PWM_HIGH_BIT, ITH_GPIO_PWM_HIGH_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM3:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, pin << ITH_GPIO_PWM_LOW_BIT, ITH_GPIO_PWM_LOW_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM4:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, pin << ITH_GPIO_PWM_HIGH_BIT, ITH_GPIO_PWM_HIGH_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM5:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, pin << ITH_GPIO_PWM_LOW_BIT, ITH_GPIO_PWM_LOW_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM6:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, pin << ITH_GPIO_PWM_HIGH_BIT, ITH_GPIO_PWM_HIGH_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM7:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, pin << ITH_GPIO_PWM_LOW_BIT, ITH_GPIO_PWM_LOW_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM8:
            ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, pin << ITH_GPIO_PWM_HIGH_BIT, ITH_GPIO_PWM_HIGH_MASK);
            ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        default:
            break;
    }
}

static void ithPwmClearGpioMode(ITHPwm pwm)
{
    switch(pwm) {
        case ITH_PWM1:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM2:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL01_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM3:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM4:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL23_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM5:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM6:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL45_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        case ITH_PWM7:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, ITH_GPIO_PWM_LOW_EN_BIT);
            break;
        case ITH_PWM8:
            ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_PWMSEL67_REG, ITH_GPIO_PWM_HIGH_EN_BIT);
            break;
        default:
            break;
    }
}

void ithPwmReset(ITHPwm pwm, unsigned int pin, unsigned int gpio_mode)
{
    ithPwmSetGpioMode(pwm, pin);
    ithGpioSetMode(pin, gpio_mode);
    ithTimerReset(pwm);
    ithTimerSetCounter(pwm, 0);
    ithTimerSetPwmMatch(pwm, blMatchs[pwm], blCounts[pwm]);

    ithTimerCtrlEnable(pwm, ITH_TIMER_UPCOUNT);
    ithTimerCtrlEnable(pwm, ITH_TIMER_PERIODIC);
    ithTimerCtrlEnable(pwm, ITH_TIMER_PWM);
    ithTimerEnable(pwm);
}

void ithPwmSetDutyCycle(ITHPwm pwm, unsigned int duty)
{
    uint32_t newmatch1 = UINT32_MAX;

    if (duty > 100)
    {
        ithPrintf("dutycycle: %u is greater than 100.\nSetting dutycycle failed!\n", duty);
    }
    else if (duty == 100)
    {
        newmatch1 = 0;
    }
    else
    {
        uint64_t temp_result = (duty == 0) ? UINT32_MAX : (uint64_t)blCounts[pwm] * (100 - duty) / 100;
        newmatch1 = (temp_result > UINT32_MAX) ? UINT32_MAX : (uint32_t)temp_result;
    }

    ithTimerSetPwmMatch(pwm, newmatch1, blCounts[pwm]);
    ithTimerCtrlEnable(pwm, ITH_TIMER_EN);
}

void ithPwmEnable(ITHPwm pwm, unsigned int pin, unsigned int gpio_mode)
{
    ithPwmSetGpioMode(pwm, pin);
    ithGpioSetMode(pin, gpio_mode);
    ithTimerCtrlEnable(pwm, ITH_TIMER_PWM);
    ithTimerCtrlEnable(pwm, ITH_TIMER_EN);
}

void ithPwmDisable(ITHPwm pwm, unsigned int pin)
{
    ithPwmClearGpioMode(pwm);
    ithGpioClear(pin);
    ithGpioEnable(pin);
    ithGpioSetOut(pin);
    ithTimerCtrlDisable(pwm, ITH_TIMER_EN);
    ithTimerCtrlDisable(pwm, ITH_TIMER_PWM);
}
