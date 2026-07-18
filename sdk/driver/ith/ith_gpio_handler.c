/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL gpio interrupt handler functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"

#define GPIO_PIN_COUNT (102UL)

static ITHGpioIntrHandler gpioHandlerTable[GPIO_PIN_COUNT];
static void *             gpioArgTable[GPIO_PIN_COUNT];

static void GpioDefaultHandler(unsigned int pin, void* arg)
{
    // DO NOTHING
}

void ithGpioInit(void)
{
    uint32_t i;

    for (i = 0UL; i < GPIO_PIN_COUNT; ++i)
    {
        gpioHandlerTable[i] = &GpioDefaultHandler;
    }
}

void ithGpioRegisterIntrHandler(unsigned int pin, ITHGpioIntrHandler handler, void* arg)
{
    if (pin < GPIO_PIN_COUNT)
    {
        gpioHandlerTable[pin] = (NULL != handler) ? handler : &GpioDefaultHandler;
        gpioArgTable[pin]     = arg;
    }
}

void ithGpioDoIntr(void)
{
    register uint32_t gpioNum   = 0UL;
    register uint32_t intrSrc1  = ithReadRegA(ITH_GPIO_BASE + ITH_GPIO1_INTRMASKSTATE_REG);   // read gpio1's intr source
    register uint32_t intrSrc2  = ithReadRegA(ITH_GPIO_BASE + ITH_GPIO2_INTRMASKSTATE_REG);   // read gpio2's intr source
    register uint32_t intrSrc3  = ithReadRegA(ITH_GPIO_BASE + ITH_GPIO3_INTRMASKSTATE_REG);   // read gpio3's intr source
    register uint32_t intrSrc4  = ithReadRegA(ITH_GPIO_BASE + ITH_GPIO4_INTRMASKSTATE_REG);   // read gpio4's intr source

    ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO1_INTRCLR_REG, intrSrc1); // clear irq1 source
    ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO2_INTRCLR_REG, intrSrc2); // clear irq2 source
    ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO3_INTRCLR_REG, intrSrc3); // clear irq3 source
    ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO4_INTRCLR_REG, intrSrc4); // clear irq4 source

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc1 != 0UL)
    {
        if ((intrSrc1 & 0xFFFFUL) == 0UL)
        {
            gpioNum += 16UL;
            intrSrc1 >>= 16UL;
        }

        if ((intrSrc1 & 0xFFUL) == 0UL)
        {
            gpioNum += 8UL;
            intrSrc1 >>= 8UL;
        }

        if ((intrSrc1 & 0xFUL) == 0UL)
        {
            gpioNum += 4UL;
            intrSrc1 >>= 4UL;
        }

        if ((intrSrc1 & 0x3UL) == 0UL)
        {
            gpioNum += 2UL;
            intrSrc1 >>= 2UL;
        }

        if ((intrSrc1 & 0x1UL) == 0UL)
        {
            gpioNum += 1UL;
            intrSrc1 >>= 1UL;
        }

        // Call the handler if gpioNum is within valid range
        if (gpioNum < GPIO_PIN_COUNT)
        {
            gpioHandlerTable[gpioNum](gpioNum, gpioArgTable[gpioNum]);
        }

        intrSrc1 &= ~1UL;
    }

    gpioNum = 32UL;

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc2 != 0UL)
    {
        if ((intrSrc2 & 0xFFFFUL) == 0UL)
        {
            gpioNum += 16UL;
            intrSrc2 >>= 16UL;
        }

        if ((intrSrc2 & 0xFFUL) == 0UL)
        {
            gpioNum += 8UL;
            intrSrc2 >>= 8UL;
        }

        if ((intrSrc2 & 0xFUL) == 0UL)
        {
            gpioNum += 4UL;
            intrSrc2 >>= 4UL;
        }

        if ((intrSrc2 & 0x3UL) == 0UL)
        {
            gpioNum += 2UL;
            intrSrc2 >>= 2UL;
        }

        if ((intrSrc2 & 0x1UL) == 0UL)
        {
            gpioNum += 1UL;
            intrSrc2 >>= 1UL;
        }

        // Call the handler if gpioNum is within valid range
        if (gpioNum < GPIO_PIN_COUNT)
        {
            gpioHandlerTable[gpioNum](gpioNum, gpioArgTable[gpioNum]);
        }

        intrSrc2 &= ~1UL;
    }

    gpioNum = 64UL;

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc3 != 0UL)
    {
        if ((intrSrc3 & 0xFFFFUL) == 0UL)
        {
            gpioNum += 16UL;
            intrSrc3 >>= 16UL;
        }

        if ((intrSrc3 & 0xFFUL) == 0UL)
        {
            gpioNum += 8UL;
            intrSrc3 >>= 8UL;
        }

        if ((intrSrc3 & 0xFUL) == 0UL)
        {
            gpioNum += 4UL;
            intrSrc3 >>= 4UL;
        }

        if ((intrSrc3 & 0x3UL) == 0UL)
        {
            gpioNum += 2UL;
            intrSrc3 >>= 2UL;
        }

        if ((intrSrc3 & 0x1UL) == 0UL)
        {
            gpioNum += 1UL;
            intrSrc3 >>= 1UL;
        }

        // Call the handler if gpioNum is within valid range
        if (gpioNum < GPIO_PIN_COUNT)
        {
            gpioHandlerTable[gpioNum](gpioNum, gpioArgTable[gpioNum]);
        }

        intrSrc3 &= ~1UL;
    }

    gpioNum = 96UL;

    // Test to see if there is a flag set, if not then don't do anything
    while (intrSrc4 != 0UL)
    {
        if ((intrSrc4 & 0xFFFFUL) == 0UL)
        {
            gpioNum += 16UL;
            intrSrc4 >>= 16UL;
        }

        if ((intrSrc4 & 0xFFUL) == 0UL)
        {
            gpioNum += 8UL;
            intrSrc4 >>= 8UL;
        }

        if ((intrSrc4 & 0xFUL) == 0UL)
        {
            gpioNum += 4UL;
            intrSrc4 >>= 4UL;
        }

        if ((intrSrc4 & 0x3UL) == 0UL)
        {
            gpioNum += 2UL;
            intrSrc4 >>= 2UL;
        }

        if ((intrSrc4 & 0x1UL) == 0UL)
        {
            gpioNum += 1UL;
            intrSrc4 >>= 1UL;
        }

        // Call the handler if gpioNum is within valid range
        if (gpioNum < GPIO_PIN_COUNT)
        {
            gpioHandlerTable[gpioNum](gpioNum, gpioArgTable[gpioNum]);
        }

        intrSrc4 &= ~1UL;
    }
}
