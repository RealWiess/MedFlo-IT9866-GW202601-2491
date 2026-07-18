/*
 * Copyright(c) 2015 ITE Tech.Inc.All Rights Reserved.
 */
/** @file
 * HAL GPIO functions.
 *
 * @author Jim Tan
 * @author Benson Lin
 * @author I-Chun Lai
 * @version 1.0
 */
#include <inttypes.h>
#include "ith_cfg.h"
#ifdef CFG_DBG_STATS_GPIO
#include <stdio.h>
#endif

static uint32_t gpioRegs[(ITH_GPIO_REV_REG - ITH_GPIO1_PINDIR_REG + 4) / 4];
#if 0
#define DEFINE_REG_ADDR_TABLE4(FUNC) static const uint32_t FUNC ## _REG_ADDR[4] =   \
{                                                                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ## _REG,                                     \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ## _REG,                                     \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ## _REG,                                     \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ## _REG,                                     \
};

#define DEFINE_REG_ADDR_TABLE7(FUNC) static const uint32_t FUNC ## _REG_ADDR[7] =   \
{                                                                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_L_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_H_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_L_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_H_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_L_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_H_REG,                                    \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ##_REG,                                      \
};

#define DEFINE_REG_ADDR_TABLE16(FUNC) static const uint32_t FUNC ## _REG_ADDR[16] = \
{                                                                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_G1_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_G2_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_G3_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO1_ ## FUNC ##_G4_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_G1_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_G2_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_G3_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO2_ ## FUNC ##_G4_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_G1_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_G2_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_G3_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO3_ ## FUNC ##_G4_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ##_G1_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ##_G2_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ##_G3_REG,                                   \
    ITH_GPIO_BASE + ITH_GPIO4_ ## FUNC ##_G4_REG,                                   \
};

DEFINE_REG_ADDR_TABLE4(PINDIR)
DEFINE_REG_ADDR_TABLE4(DATASET)
DEFINE_REG_ADDR_TABLE4(DATACLR)
DEFINE_REG_ADDR_TABLE4(DATAIN)
DEFINE_REG_ADDR_TABLE4(INTREN)
DEFINE_REG_ADDR_TABLE4(INTRCLR)
DEFINE_REG_ADDR_TABLE4(BOUNCEEN)
DEFINE_REG_ADDR_TABLE4(PULLEN)
DEFINE_REG_ADDR_TABLE4(PULLTYPE)
DEFINE_REG_ADDR_TABLE4(INTRTRIG)
DEFINE_REG_ADDR_TABLE4(INTRBOTH)
DEFINE_REG_ADDR_TABLE4(INTRRISENEG)
DEFINE_REG_ADDR_TABLE16(MODESEL)
DEFINE_REG_ADDR_TABLE7(DRIVING_SET)
#endif
static const uint32_t PINDIR_REG_ADDR[4]        = { ITH_GPIO_BASE + ITH_GPIO1_PINDIR_REG, ITH_GPIO_BASE + ITH_GPIO2_PINDIR_REG, ITH_GPIO_BASE + ITH_GPIO3_PINDIR_REG, ITH_GPIO_BASE + ITH_GPIO4_PINDIR_REG };
static const uint32_t DATASET_REG_ADDR[4]       = { ITH_GPIO_BASE + ITH_GPIO1_DATASET_REG, ITH_GPIO_BASE + ITH_GPIO2_DATASET_REG, ITH_GPIO_BASE + ITH_GPIO3_DATASET_REG, ITH_GPIO_BASE + ITH_GPIO4_DATASET_REG };
static const uint32_t DATACLR_REG_ADDR[4]       = { ITH_GPIO_BASE + ITH_GPIO1_DATACLR_REG, ITH_GPIO_BASE + ITH_GPIO2_DATACLR_REG, ITH_GPIO_BASE + ITH_GPIO3_DATACLR_REG, ITH_GPIO_BASE + ITH_GPIO4_DATACLR_REG };
static const uint32_t DATAIN_REG_ADDR[4]        = { ITH_GPIO_BASE + ITH_GPIO1_DATAIN_REG, ITH_GPIO_BASE + ITH_GPIO2_DATAIN_REG, ITH_GPIO_BASE + ITH_GPIO3_DATAIN_REG, ITH_GPIO_BASE + ITH_GPIO4_DATAIN_REG };
static const uint32_t INTREN_REG_ADDR[4]        = { ITH_GPIO_BASE + ITH_GPIO1_INTREN_REG, ITH_GPIO_BASE + ITH_GPIO2_INTREN_REG, ITH_GPIO_BASE + ITH_GPIO3_INTREN_REG, ITH_GPIO_BASE + ITH_GPIO4_INTREN_REG };
static const uint32_t INTRCLR_REG_ADDR[4]       = { ITH_GPIO_BASE + ITH_GPIO1_INTRCLR_REG, ITH_GPIO_BASE + ITH_GPIO2_INTRCLR_REG, ITH_GPIO_BASE + ITH_GPIO3_INTRCLR_REG, ITH_GPIO_BASE + ITH_GPIO4_INTRCLR_REG };
static const uint32_t BOUNCEEN_REG_ADDR[4]      = { ITH_GPIO_BASE + ITH_GPIO1_BOUNCEEN_REG, ITH_GPIO_BASE + ITH_GPIO2_BOUNCEEN_REG, ITH_GPIO_BASE + ITH_GPIO3_BOUNCEEN_REG, ITH_GPIO_BASE + ITH_GPIO4_BOUNCEEN_REG };
static const uint32_t PULLEN_REG_ADDR[4]        = { ITH_GPIO_BASE + ITH_GPIO1_PULLEN_REG, ITH_GPIO_BASE + ITH_GPIO2_PULLEN_REG, ITH_GPIO_BASE + ITH_GPIO3_PULLEN_REG, ITH_GPIO_BASE + ITH_GPIO4_PULLEN_REG };
static const uint32_t PULLTYPE_REG_ADDR[4]      = { ITH_GPIO_BASE + ITH_GPIO1_PULLTYPE_REG, ITH_GPIO_BASE + ITH_GPIO2_PULLTYPE_REG, ITH_GPIO_BASE + ITH_GPIO3_PULLTYPE_REG, ITH_GPIO_BASE + ITH_GPIO4_PULLTYPE_REG };
static const uint32_t INTRTRIG_REG_ADDR[4]      = { ITH_GPIO_BASE + ITH_GPIO1_INTRTRIG_REG, ITH_GPIO_BASE + ITH_GPIO2_INTRTRIG_REG, ITH_GPIO_BASE + ITH_GPIO3_INTRTRIG_REG, ITH_GPIO_BASE + ITH_GPIO4_INTRTRIG_REG };
static const uint32_t INTRBOTH_REG_ADDR[4]      = { ITH_GPIO_BASE + ITH_GPIO1_INTRBOTH_REG, ITH_GPIO_BASE + ITH_GPIO2_INTRBOTH_REG, ITH_GPIO_BASE + ITH_GPIO3_INTRBOTH_REG, ITH_GPIO_BASE + ITH_GPIO4_INTRBOTH_REG };
static const uint32_t INTRRISENEG_REG_ADDR[4]   = { ITH_GPIO_BASE + ITH_GPIO1_INTRRISENEG_REG, ITH_GPIO_BASE + ITH_GPIO2_INTRRISENEG_REG, ITH_GPIO_BASE + ITH_GPIO3_INTRRISENEG_REG, ITH_GPIO_BASE + ITH_GPIO4_INTRRISENEG_REG };
static const uint32_t MODESEL_REG_ADDR[16]      = { ITH_GPIO_BASE + ITH_GPIO1_MODESEL_G1_REG, ITH_GPIO_BASE + ITH_GPIO1_MODESEL_G2_REG, ITH_GPIO_BASE + ITH_GPIO1_MODESEL_G3_REG, ITH_GPIO_BASE + ITH_GPIO1_MODESEL_G4_REG,
                                                    ITH_GPIO_BASE + ITH_GPIO2_MODESEL_G1_REG, ITH_GPIO_BASE + ITH_GPIO2_MODESEL_G2_REG, ITH_GPIO_BASE + ITH_GPIO2_MODESEL_G3_REG, ITH_GPIO_BASE + ITH_GPIO2_MODESEL_G4_REG,
                                                    ITH_GPIO_BASE + ITH_GPIO3_MODESEL_G1_REG, ITH_GPIO_BASE + ITH_GPIO3_MODESEL_G2_REG, ITH_GPIO_BASE + ITH_GPIO3_MODESEL_G3_REG, ITH_GPIO_BASE + ITH_GPIO3_MODESEL_G4_REG,
                                                    ITH_GPIO_BASE + ITH_GPIO4_MODESEL_G1_REG, ITH_GPIO_BASE + ITH_GPIO4_MODESEL_G2_REG, ITH_GPIO_BASE + ITH_GPIO4_MODESEL_G3_REG, ITH_GPIO_BASE + ITH_GPIO4_MODESEL_G4_REG };
static const uint32_t DRIVING_SET_REG_ADDR[7]   = { ITH_GPIO_BASE + ITH_GPIO1_DRIVING_SET_L_REG, ITH_GPIO_BASE + ITH_GPIO1_DRIVING_SET_H_REG, ITH_GPIO_BASE + ITH_GPIO2_DRIVING_SET_L_REG, ITH_GPIO_BASE + ITH_GPIO2_DRIVING_SET_H_REG,
                                                    ITH_GPIO_BASE + ITH_GPIO3_DRIVING_SET_L_REG, ITH_GPIO_BASE + ITH_GPIO3_DRIVING_SET_H_REG, ITH_GPIO_BASE + ITH_GPIO4_DRIVING_SET_REG };
static const uint32_t FFINEN_REG_ADDR[4]        = { ITH_GPIO_BASE + ITH_GPIO1_FFIEN_REG, ITH_GPIO_BASE + ITH_GPIO2_FFIEN_REG, ITH_GPIO_BASE + ITH_GPIO3_FFIEN_REG, ITH_GPIO_BASE + ITH_GPIO4_FFIEN_REG };

/*
The above code is equal to:
static const uint32_t PINDIR_REG_ADDR[4]        = { 0xD1000000 + 0x08,  0xD1000000 + 0x88,  0xD1000000 + 0x108, 0xD1000000 + 0x188 };
static const uint32_t DATASET_REG_ADDR[4]       = { 0xD1000000 + 0x0C,  0xD1000000 + 0x8C,  0xD1000000 + 0x10C, 0xD1000000 + 0x18C };
static const uint32_t DATACLR_REG_ADDR[4]       = { 0xD1000000 + 0x10,  0xD1000000 + 0x90,  0xD1000000 + 0x110, 0xD1000000 + 0x190 };
static const uint32_t DATAIN_REG_ADDR[4]        = { 0xD1000000 + 0x04,  0xD1000000 + 0x84,  0xD1000000 + 0x104, 0xD1000000 + 0x184 };
static const uint32_t INTREN_REG_ADDR[4]        = { 0xD1000000 + 0x1C,  0xD1000000 + 0x9C,  0xD1000000 + 0x11C, 0xD1000000 + 0x19C };
static const uint32_t INTRCLR_REG_ADDR[4]       = { 0xD1000000 + 0x2C,  0xD1000000 + 0xAC,  0xD1000000 + 0x12C, 0xD1000000 + 0x1AC };
static const uint32_t BOUNCEEN_REG_ADDR[4]      = { 0xD1000000 + 0x3C,  0xD1000000 + 0xBC,  0xD1000000 + 0x13C, 0xD1000000 + 0x1BC };
static const uint32_t PULLEN_REG_ADDR[4]        = { 0xD1000000 + 0x14,  0xD1000000 + 0x94,  0xD1000000 + 0x114, 0xD1000000 + 0x194 };
static const uint32_t PULLTYPE_REG_ADDR[4]      = { 0xD1000000 + 0x18,  0xD1000000 + 0x98,  0xD1000000 + 0x118, 0xD1000000 + 0x198 };
static const uint32_t INTRTRIG_REG_ADDR[4]      = { 0xD1000000 + 0x30,  0xD1000000 + 0xB0,  0xD1000000 + 0x130, 0xD1000000 + 0x1B0 };
static const uint32_t INTRBOTH_REG_ADDR[4]      = { 0xD1000000 + 0x34,  0xD1000000 + 0xB4,  0xD1000000 + 0x134, 0xD1000000 + 0x1B4 };
static const uint32_t INTRRISENEG_REG_ADDR[4]   = { 0xD1000000 + 0x38,  0xD1000000 + 0xB8,  0xD1000000 + 0x138, 0xD1000000 + 0x1B8 };
static const uint32_t MODESEL_REG_ADDR[16]      = { 0xD1000000 + 0x60,  0xD1000000 + 0x64,  0xD1000000 + 0x68,  0xD1000000 + 0x6C,
                                                    0xD1000000 + 0xE0,  0xD1000000 + 0xE4,  0xD1000000 + 0xE8,  0xD1000000 + 0xEC,
                                                    0xD1000000 + 0x160, 0xD1000000 + 0x164, 0xD1000000 + 0x168, 0xD1000000 + 0x16C,
                                                    0xD1000000 + 0x1E0, 0xD1000000 + 0x1E4, 0xD1000000 + 0x1E8, 0xD1000000 + 0x1EC };
static const uint32_t DRIVING_SET_REG_ADDR[7]   = { 0xD1000000 + 0x50,  0xD1000000 + 0x54,  0xD1000000 + 0xD0,  0xD1000000 + 0xD4,
                                                    0xD1000000 + 0x150, 0xD1000000 + 0x154, 0xD1000000 + 0x1D0 };
*/

void ithGpioSetMode(unsigned int pin, ITHGpioMode mode)
{
    uint32_t value, mask;

    ithEnterCritical();

    switch (mode)
    {
    case ITH_GPIO_MODE_RX2WGAND:
        ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_MISC0_REG, ITH_GPIO_UR2_RXSRC_BIT);
        mode = 0;
        break;

    case ITH_GPIO_MODE_RX3WGAND:
        ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_MISC0_REG, ITH_GPIO_UR3_RXSRC_BIT);
        mode = 0;
        break;

    default:
        /* do nothing */
        break;
    }

    value = mode << ((pin & 0x7UL) * 4UL);
    mask  = 0x0FUL << ((pin & 0x7UL) * 4UL);
    if (pin < 96UL)
    {
        ithWriteRegMaskA(MODESEL_REG_ADDR[pin >> 3UL], value, mask);
    }
    #if 0
    else
    {
        // workaround: Register ITH_GPIO7_MODE_REG is write only (IC defact).
        // So we need a temp variable to store the GPIO7 mode setting.
        static uint16_t gpio7ModeRegData = 0U;
        gpio7ModeRegData = ((gpio7ModeRegData & ~mask) | (value & mask));
        ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO7_MODE_REG, gpio7ModeRegData);
    }
    #endif

    ithExitCritical();
}

void ithGpioCtrlEnable(unsigned int pin, ITHGpioCtrl ctrl)
{
     uint32_t port = pin >> 5UL;

    ithEnterCritical();

    switch (ctrl)
    {
    case ITH_GPIO_PULL_ENABLE:
        ithSetRegBitA(PULLEN_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_PULL_UP:
        ithSetRegBitA(PULLTYPE_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_LEVELTRIGGER:
        ithSetRegBitA(INTRTRIG_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_BOTHEDGE:
        ithSetRegBitA(INTRBOTH_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_TRIGGERFALLING:
        ithSetRegBitA(INTRRISENEG_REG_ADDR[port], (pin & 0x1FUL));
        break;

    default:
        /* do nothing */
        break;
    }

    ithExitCritical();
}

void ithGpioCtrlDisable(unsigned int pin, ITHGpioCtrl ctrl)
{
    uint32_t port = pin >> 5UL;

    ithEnterCritical();

    switch (ctrl)
    {
    case ITH_GPIO_PULL_ENABLE:
        ithClearRegBitA(PULLEN_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_PULL_UP:
        ithClearRegBitA(PULLTYPE_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_LEVELTRIGGER:
        ithClearRegBitA(INTRTRIG_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_BOTHEDGE:
        ithClearRegBitA(INTRBOTH_REG_ADDR[port], (pin & 0x1FUL));
        break;

    case ITH_GPIO_INTR_TRIGGERFALLING:
        ithClearRegBitA(INTRRISENEG_REG_ADDR[port], (pin & 0x1FUL));
        break;

    default:
        /* do nothing */
        break;
    }

    ithExitCritical();
}

void ithGpioSuspend(void)
{
    uint32_t i = 0UL;

    for (i = 0UL; i < ITH_COUNT_OF(gpioRegs); i++)
    {
        switch (i)
        {
        case ITH_GPIO1_DATASET_REG:
        case ITH_GPIO1_DATACLR_REG:
        case ITH_GPIO1_INTRRAWSTATE_REG:
        case ITH_GPIO1_INTRMASKSTATE_REG:
        case ITH_GPIO1_INTRCLR_REG:
        case ITH_GPIO2_DATAOUT_REG:
        case ITH_GPIO2_DATAIN_REG:
        case ITH_GPIO2_DATASET_REG:
        case ITH_GPIO2_DATACLR_REG:
        case ITH_GPIO2_INTRRAWSTATE_REG:
        case ITH_GPIO2_INTRMASKSTATE_REG:
        case ITH_GPIO2_INTRCLR_REG:
        case ITH_GPIO_FEATURE_REG:
        case ITH_GPIO_REV_REG:
            // don't need to backup
            break;

        default:
            gpioRegs[i] = ithReadRegA(ITH_GPIO_BASE + ITH_GPIO1_PINDIR_REG + (i *4UL));
            break;
        }
    }
}

void ithGpioResume(void)
{
    uint32_t i = 0UL;

    for (i = 0UL; i < ITH_COUNT_OF(gpioRegs); i++)
    {
        switch (i)
        {
        case ITH_GPIO1_DATASET_REG:
        case ITH_GPIO1_DATACLR_REG:
        case ITH_GPIO1_INTRRAWSTATE_REG:
        case ITH_GPIO1_INTRMASKSTATE_REG:
        case ITH_GPIO1_INTRCLR_REG:
        case ITH_GPIO2_DATAOUT_REG:
        case ITH_GPIO2_DATAIN_REG:
        case ITH_GPIO2_DATASET_REG:
        case ITH_GPIO2_DATACLR_REG:
        case ITH_GPIO2_INTRRAWSTATE_REG:
        case ITH_GPIO2_INTRMASKSTATE_REG:
        case ITH_GPIO2_INTRCLR_REG:
        case ITH_GPIO_FEATURE_REG:
        case ITH_GPIO_REV_REG:
            // don't need to restore
            break;

        default:
            ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO1_PINDIR_REG + (i * 4UL), gpioRegs[i]);
            break;
        }
    }
}

void ithGpioEnableClock(void)
{
    // enable clock
    ithSetRegBitA(ITH_APB_CLK2_REG, ITH_EN_W1CLK_BIT);
}

void ithGpioDisableClock(void)
{
    // disable clock
    ithClearRegBitA(ITH_APB_CLK2_REG, ITH_EN_W1CLK_BIT);
}

#ifdef CFG_DBG_STATS_GPIO
void ithGpioStatusDbg(void)
{
    int gpioPinStart = CFG_DBG_STATS_GPIO_START_PIN;
    int gpioPinEnd = CFG_DBG_STATS_GPIO_END_PIN;
    uint32_t i = 0UL;

    (void)printf("GPIO Status:\n");

    if ((gpioPinStart < 0) || (gpioPinEnd < 0) || (gpioPinStart > gpioPinEnd))
    {
        (void)printf("ERROR CFG : DBG GPIO start or end pin error.\n");
        return;
    }

    for (i = ((uint32_t)gpioPinStart); i <= ((uint32_t)gpioPinEnd); i++)
    {
        uint32_t mode = (ithReadRegA(MODESEL_REG_ADDR[i >> 3UL]) & (0x0FUL << ((i & 0x7UL) * 4UL))) >> ((i & 0x7UL) * 4UL);
        uint32_t io = (ithReadRegA(PINDIR_REG_ADDR[i >> 5UL]) & (0x01UL << (i & 0x1FUL))) >> (i & 0x1FUL);
        (void)printf("GPIO[%" PRIu32 "] mode=%" PRIu32 ", io=%" PRIu32 ", val=%" PRIu32 "\n", i, mode, io, ithGpioGet(i) >> (i & 0x1FUL));
    }
}
#endif // CFG_DBG_STATS_GPIO

void ithGpioSetDriving(unsigned int pin, ITHGpioDriving level)
{
    ithEnterCritical();
    ithWriteRegMaskA(DRIVING_SET_REG_ADDR[pin >> 4UL],
                     level << ((pin & 0xFUL) * 2UL),
                     (0x3UL) << ((pin & 0xFUL) * 2UL));
    ithExitCritical();
}

void ithGpioSetIn(unsigned int pin)
{
    uint32_t port = pin >> 5UL;

    ithEnterCritical();
    ithClearRegBitA(PINDIR_REG_ADDR[port], pin & 0x1FUL);
    ithExitCritical();
}

void ithGpioSetOut(unsigned int pin)
{
    uint32_t port = pin >> 5UL;

    ithEnterCritical();
    ithSetRegBitA(PINDIR_REG_ADDR[port], pin & 0x1FUL);
    ithExitCritical();
}

void ithGpioSet(unsigned int pin)
{
    uint32_t port = pin >> 5UL;

    ithEnterCritical();
    ithWriteRegA(DATASET_REG_ADDR[port], 0x1UL << (pin & 0x1FUL));
    ithExitCritical();
}

void ithGpioClear(unsigned int pin)
{
    uint32_t port = pin >> 5UL;

    ithEnterCritical();
    ithWriteRegA(DATACLR_REG_ADDR[port], 0x1UL << (pin & 0x1FUL));
    ithExitCritical();
}

uint32_t ithGpioGet(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    return ithReadRegA(DATAIN_REG_ADDR[port]) & (0x1UL << (pin & 0x1FUL));
}

void ithGpioEnableIntr(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithSetRegBitA(INTREN_REG_ADDR[port], (pin & 0x1FUL));
}

void ithGpioDisableIntr(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithClearRegBitA(INTREN_REG_ADDR[port], (pin & 0x1FUL));
}

void ithGpioClearIntr(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithSetRegBitA(INTRCLR_REG_ADDR[port], (pin & 0x1FUL));
}

void ithGpioEnableBounce(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithSetRegBitA(BOUNCEEN_REG_ADDR[port], (pin & 0x1FUL));
}

void ithGpioDisableBounce(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithClearRegBitA(BOUNCEEN_REG_ADDR[port], (pin & 0x1FUL));
}

void ithGpioSetDebounceClock(unsigned int clk)
{
    ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO_BOUNCEPRESCALE_REG, (ithGetBusClock() / clk) - 1UL);
}

void ithGpioSetFFIEn(unsigned int pin)
{
    uint32_t port = pin >> 5UL;
    ithEnterCritical();
    ithSetRegBitA(FFINEN_REG_ADDR[port], (pin & 0x1FUL));
    ithExitCritical();
}
