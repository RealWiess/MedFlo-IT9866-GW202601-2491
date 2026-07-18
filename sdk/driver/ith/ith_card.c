/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL Card functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <string.h>
#include <unistd.h>
#include "ith_cfg.h"

static ITHCardConfig cardCfg;

void ithCardInit(const ITHCardConfig *cfg)
{
    (void)memcpy(&cardCfg, cfg, sizeof(ITHCardConfig));
}

void ithCardPowerOn(ITHCardPin pin)
{
#ifndef CFG_ITH_FPGA
    unsigned int num = cardCfg.powerEnablePins[pin];
    if (num == (unsigned char)-1) {
        goto end;
    }

    ithGpioEnable(num);
    ithGpioSetOut(num);
    ithGpioClear(num);
#endif // !CFG_ITH_FPGA

end:
    return;
}

void ithCardPowerOff(ITHCardPin pin)
{
#ifndef CFG_ITH_FPGA
    unsigned int num = cardCfg.powerEnablePins[pin];
    if (num == (unsigned char)-1) {
        goto end;
    }

    ithGpioEnable(num);
    ithGpioSetOut(num);
    ithGpioSet(num);
#endif // !CFG_ITH_FPGA

end:
    return;
}

#if defined (CFG_GPIO_SD_WIFI_POWER_ENABLE) && defined(CFG_NET_WIFI_SDIO_VND_RTK)
#define _CHECK_WIFI_PIN_NUM_SAFE(x) ((x > 0) ? x : 0)
#define WIFI_PWR_PIN     _CHECK_WIFI_PIN_NUM_SAFE(CFG_GPIO_SD_WIFI_POWER_PIN)
#define WIFI_REG_PIN     _CHECK_WIFI_PIN_NUM_SAFE(CFG_GPIO_SD_WIFI_WL_REG_PIN)

/* For RTK HW */
void ithWIFICardPullHighFromPin(unsigned int pin)
{
    if (pin == 0)
        return;

    ithGpioEnable(pin);
    ithGpioSetOut(pin);
    ithGpioSet(pin); //GPIO pin pull high
}

void ithWIFICardPullLowFromPin(unsigned int pin)
{
    if (pin == 0)
        return;

    ithGpioEnable(pin);
    ithGpioSetOut(pin);
    ithGpioClear(pin); //GPIO pin pull low
}

#define NEW_PWR_SEQ 0
/* Power-on sequence for RTK modules */
void ithWIFICardPowerProcess(void)
{
    (void)ithPrintf("==== Power Sequence of WIFI ===\n");
#if NEW_PWR_SEQ

    /* Power pin pull high (power down) */
    ithWIFICardPullHighFromPin((unsigned int)WIFI_PWR_PIN);
    ithDelay(250*1000);
    /* Power pin pull low (power up) */
    ithWIFICardPullLowFromPin((unsigned int)WIFI_PWR_PIN);
    ithDelay(2*1000);

#else

    /* WL_REGON pull high */
    ithWIFICardPullHighFromPin((unsigned int)WIFI_REG_PIN);
    ithDelay(10);
    /* WL_REGON pull low */
    ithWIFICardPullLowFromPin((unsigned int)WIFI_REG_PIN);

    /* Power pin pull high (power down) */
    ithWIFICardPullHighFromPin((unsigned int)WIFI_PWR_PIN);
    ithDelay(250*1000);
    /* Power pin pull low (power up) */
    ithWIFICardPullLowFromPin((unsigned int)WIFI_PWR_PIN);
    ithDelay(100);

    /* WL_REGON pull high */
    ithWIFICardPullHighFromPin((unsigned int)WIFI_REG_PIN);

#endif
}

#endif

bool ithCardInserted(ITHCardPin pin)
{
#ifdef CFG_ITH_FPGA
    return true;
#else
    unsigned int num = cardCfg.cardDetectPins[pin];
    bool inserted = false;

    if (num == (unsigned char)-1) {
        inserted = false;
        goto end;
    }
    // always insert
    if (num == (unsigned char)-2) {
        inserted = true;
        goto end;
    }

    ithGpioEnable(num);
    ithGpioSetIn(num);

    if (pin == ITH_CARDPIN_SD0)
    {
    #if defined(CFG_SD0_CARD_DETECT_ACTIVE_HIGH)
        inserted = ithGpioGet(num) ? true : false;
        goto end;
    #endif
    }
    if (pin == ITH_CARDPIN_SD1)
    {
    #if defined(CFG_SD1_CARD_DETECT_ACTIVE_HIGH)
        inserted = ithGpioGet(num) ? true : false;
        goto end;
    #endif
    }

    /* active low */
    inserted = ithGpioGet(num) ? false : true;

end:
    return inserted;
#endif // CFG_ITH_FPGA
}

bool ithCardLocked(ITHCardPin pin)
{
#ifdef CFG_ITH_FPGA
    return false;
#else
    unsigned int num;
    bool lock;

    num = cardCfg.writeProtectPins[pin];
    if (num == (unsigned char)-1) {
        lock = false;
        goto end;
    }

    ithGpioEnable(num);
    ithGpioSetIn(num);
    lock = ithGpioGet(num) ? true : false;

end:
    return lock;
#endif // CFG_ITH_FPGA
}


#define SET_SD_DATA_MODE(x)    ((x & 0x7) << 0)
#define SET_SD_CLK_MODE(x)     ((x & 0x7) << 4)
#define SET_SD_PIN_IDX(x)      ((x & 0xF) << 8)  /* 0:D0, ~ 7:D7, 8:CMD */
#define SET_SDC(idx)           (idx << 12)

#define GET_SD_DATA_MODE(gpio) ((sd_gpio_tlb[gpio] >> 0) & 0x7)
#define GET_SD_CLK_MODE(gpio)  ((sd_gpio_tlb[gpio] >> 4) & 0x7)
#define GET_SD_PIN_INDEX(gpio) ((sd_gpio_tlb[gpio] >> 8) & 0xF)
#define GET_SDC(gpio)          ((sd_gpio_tlb[gpio] >> 12) & 0x1)


static uint16_t sd_gpio_tlb[64] = {
    SET_SDC(0) | SET_SD_PIN_IDX(4) | SET_SD_DATA_MODE(2),                       // gpio 0
    SET_SDC(0) | SET_SD_PIN_IDX(5) | SET_SD_DATA_MODE(2),                       // gpio 1
    SET_SDC(0) | SET_SD_PIN_IDX(6) | SET_SD_DATA_MODE(2),                       // gpio 2
    SET_SDC(0) | SET_SD_PIN_IDX(7) | SET_SD_DATA_MODE(2),                       // gpio 3
    SET_SDC(1) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 4
    0,                                                                          // gpio 5
    SET_SDC(0) | SET_SD_PIN_IDX(0) | SET_SD_DATA_MODE(2),                       // gpio 6
    SET_SDC(0) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 7
    SET_SDC(0) | SET_SD_PIN_IDX(2) | SET_SD_DATA_MODE(2),                       // gpio 8
    SET_SDC(0) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(2),                       // gpio 9
    SET_SDC(0) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(2),                       // gpio 10
    SET_SD_CLK_MODE(2),                                                         // gpio 11
    SET_SD_CLK_MODE(2),                                                         // gpio 12
    SET_SDC(0) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(2),                       // gpio 13
    SET_SD_CLK_MODE(2),                                                         // gpio 14
    SET_SDC(0) | SET_SD_PIN_IDX(0) | SET_SD_DATA_MODE(2),                       // gpio 15
    SET_SDC(0) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 16
    SET_SDC(0) | SET_SD_PIN_IDX(2) | SET_SD_DATA_MODE(2),                       // gpio 17
    SET_SDC(0) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(2),                       // gpio 18
    SET_SDC(1) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 19
    SET_SDC(1) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(2),                      // gpio 20
    0,                                                                          // gpio 21
    0,                                                                          // gpio 22
    0,                                                                          // gpio 23
    0,                                                                          // gpio 24
    0,                                                                          // gpio 25
    0,                                                                          // gpio 26
    SET_SDC(0) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 27
    SET_SDC(0) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(2),                       // gpio 28
    SET_SD_CLK_MODE(2),                                                         // gpio 29
    0,                                                                          // gpio 30
    0,                                                                          // gpio 31
    SET_SD_CLK_MODE(2),                                                         // gpio 32
    SET_SDC(1) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(2),                       // gpio 33
    SET_SDC(1) | SET_SD_PIN_IDX(0) | SET_SD_DATA_MODE(2),                       // gpio 34
    SET_SDC(1) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(2),                       // gpio 35
    SET_SDC(1) | SET_SD_PIN_IDX(2) | SET_SD_DATA_MODE(2),                       // gpio 36
    SET_SDC(1) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(2),                       // gpio 37
    0,                                                                          // gpio 38
    0,                                                                          // gpio 39
    0,                                                                          // gpio 40
    0,                                                                          // gpio 41
    SET_SD_CLK_MODE(4),                                                         // gpio 42
    SET_SDC(1) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(4),                       // gpio 43
    SET_SDC(1) | SET_SD_PIN_IDX(0) | SET_SD_DATA_MODE(4),                       // gpio 44
    SET_SDC(1) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(4),                       // gpio 45
    SET_SDC(1) | SET_SD_PIN_IDX(2) | SET_SD_DATA_MODE(4),                       // gpio 46
    SET_SDC(1) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(4),                       // gpio 47
    SET_SDC(1) | SET_SD_PIN_IDX(4) | SET_SD_DATA_MODE(4),                       // gpio 48
    SET_SDC(1) | SET_SD_PIN_IDX(5) | SET_SD_DATA_MODE(4),                       // gpio 49
    0,                                                                          // gpio 50
    SET_SDC(1) | SET_SD_PIN_IDX(6) | SET_SD_DATA_MODE(4) | SET_SD_CLK_MODE(3),  // gpio 51
    SET_SDC(0) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(3),                       // gpio 52  // ???????
    SET_SDC(0) | SET_SD_PIN_IDX(0) | SET_SD_DATA_MODE(3),                       // gpio 53
    SET_SDC(0) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(3),                       // gpio 54
    SET_SDC(0) | SET_SD_PIN_IDX(2) | SET_SD_DATA_MODE(3),                       // gpio 55
    SET_SDC(0) | SET_SD_PIN_IDX(3) | SET_SD_DATA_MODE(3),                       // gpio 56
    SET_SDC(0) | SET_SD_PIN_IDX(4) | SET_SD_DATA_MODE(3),                       // gpio 57
    SET_SDC(0) | SET_SD_PIN_IDX(5) | SET_SD_DATA_MODE(3),                       // gpio 58
    SET_SDC(0) | SET_SD_PIN_IDX(6) | SET_SD_DATA_MODE(3),                       // gpio 59
    SET_SDC(0) | SET_SD_PIN_IDX(7) | SET_SD_DATA_MODE(3),                       // gpio 60
    SET_SDC(1) | SET_SD_PIN_IDX(1) | SET_SD_DATA_MODE(3),                       // gpio 61
    SET_SDC(1) | SET_SD_PIN_IDX(8) | SET_SD_DATA_MODE(3),                       // gpio 62
    SET_SD_CLK_MODE(3)                                                          // gpio 63
};

static uint16_t sd_gpio_52 = SET_SDC(1) | SET_SD_PIN_IDX(7) | SET_SD_DATA_MODE(4);  // gpio 52 for SD1_D7

#define GET_SD_DATA_MODE_52(gpio) ((sd_gpio_52 >> 0) & 0x7)
#define GET_SD_CLK_MODE_52(gpio)  ((sd_gpio_52 >> 4) & 0x7)
#define GET_SD_PIN_INDEX_52(gpio) ((sd_gpio_52 >> 8) & 0xF)
#define GET_SDC_52(gpio)          ((sd_gpio_52 >> 12) & 0x1)


#if defined(CFG_SD0_NO_PIN_SHARE)
static bool sd0_select = false;
#endif
#if defined(CFG_SD1_NO_PIN_SHARE)
static bool sd1_select = false;
#endif

void ithStorageUnSelect(ITHStorage storage)
{
    int           i = 0;
    unsigned int num;

    switch (storage)
    {
    case ITH_STOR_SD:
        for (i = 0; i < SD_PIN_NUM; i++)
        {
            num = cardCfg.sd0Pins[i];
            if (num != (unsigned char)-1)
            {
                ithGpioSetMode(num, ITH_GPIO_MODE0);            /* set input mode */
                ithGpioCtrlDisable(num, ITH_GPIO_PULL_ENABLE);  /* sd io pull disable */
            }
        }
        #if defined(CFG_SD0_NO_PIN_SHARE)
        sd0_select = false;
        #endif
        break;
    case ITH_STOR_SD1:
        for (i = 0; i < SD_PIN_NUM; i++)
        {
            num = cardCfg.sd1Pins[i];
            if (num != (unsigned char)-1)
            {
                ithGpioSetMode(num, ITH_GPIO_MODE0);            /* set input mode */
                ithGpioCtrlDisable(num, ITH_GPIO_PULL_ENABLE);  /* sd io pull disable */
            }
        }
        #if defined(CFG_SD1_NO_PIN_SHARE)
        sd1_select = false;
        #endif
        break;
    default:
        break;
    }

    return;
}

void ithStorageSelect(ITHStorage storage)
{
    int           i = 0;
    unsigned int num;
    unsigned char mode;
    uint32_t      sd_pin_sel = 0;

    #if defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_NO_PIN_SHARE)
    /* disable sd0 clock */
    num = cardCfg.sd0Pins[0];
    if (num != (unsigned char)-1) {
        ithGpioSetMode(num, ITH_GPIO_MODE0);
    }
    /* switch cmd to gpio mode,
       avoid 2 gpio with same controller cause command response will always or 1. */
    num = cardCfg.sd0Pins[1];
    ithGpioSetMode(num, ITH_GPIO_MODE0);
    #endif

    #if defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_NO_PIN_SHARE)
    /* disable sd1 clock */
    num = cardCfg.sd1Pins[0];
    if (num != (unsigned char)-1) {
        ithGpioSetMode(num, ITH_GPIO_MODE0);
    }
    /* switch cmd to gpio mode,
       avoid 2 gpio with same controller cause command response will always or 1. */
    num = cardCfg.sd1Pins[1];
    ithGpioSetMode(num, ITH_GPIO_MODE0);
    #endif

    switch (storage)
    {
    case ITH_STOR_SD:
        {
            #if defined(CFG_SD0_NO_PIN_SHARE)
            if (sd0_select) {
                goto end;
            }
            #endif
            /* switch to sd0 io */
            //for (i = 0; i < SD_PIN_NUM; i++)
            for (i = (SD_PIN_NUM - 1); i >= 0; i--) /* clock is the last to switch back */
            {
                num = cardCfg.sd0Pins[i];
                if (num != (unsigned char)-1)
                {
                #if (CFG_CHIP_FAMILY == 9860)
                    if (i == 0) {
                        mode = GET_SD_CLK_MODE(num);
                    }
                    else if ((i == 9) && (num == 52)) {
                        mode = GET_SD_DATA_MODE_52(num);
                    }
                    else {
                        mode = GET_SD_DATA_MODE(num);
                    }
                #else
                    mode = (i == 0) ? GET_SD_CLK_MODE(num) : GET_SD_DATA_MODE(num);
                #endif
                    ithGpioSetMode(num, mode);
                    //ithGpioCtrlEnable(num, ITH_GPIO_PULL_ENABLE);      /* sd io pull up */
                    //ithGpioCtrlEnable(num, ITH_GPIO_PULL_UP);
                    ithGpioSetDriving(num, ITH_GPIO_DRIVING_1);
                }
                if ((i != 0) && (num != (unsigned char)-1)) {
                #if (CFG_CHIP_FAMILY == 9860)
                    if ((i == 9) && (num == 52)) {
                        sd_pin_sel |= (GET_SDC_52(num) == 0) ? 0 : (1 << GET_SD_PIN_INDEX_52(num));
                    }
                    else {
                        sd_pin_sel |= (GET_SDC(num) == 0) ? 0 : (1 << GET_SD_PIN_INDEX(num));
                    }
                #else
                    sd_pin_sel |= (GET_SDC(num) == 0) ? 0 : (1 << GET_SD_PIN_INDEX(num));
                #endif
                }
            }
            ithWriteRegA(ITH_SDG_BASE + 0x0, sd_pin_sel);

            #if defined(CFG_SD0_NO_PIN_SHARE)
            sd0_select = true;
            #endif
        }
        break;
    case ITH_STOR_SD1:
        {
            #if defined(CFG_SD1_NO_PIN_SHARE)
            if (sd1_select) {
                goto end;
            }
            #endif
            /* switch to sd1 io */
            // for (i = 0; i < SD_PIN_NUM; i++)
            for (i = (SD_PIN_NUM - 1); i >= 0; i--) /* clock is the last to switch back */
            {
                num = cardCfg.sd1Pins[i];
                if (num != (unsigned char)-1)
                {
                #if (CFG_CHIP_FAMILY == 9860)
                    if (i == 0) {
                        mode = GET_SD_CLK_MODE(num);
                    }
                    else if ((i == 9) && (num == 52)) {
                        mode = GET_SD_DATA_MODE_52(num);
                    }
                    else {
                        mode = GET_SD_DATA_MODE(num);
                    }
                #else
                    mode = (i == 0) ? GET_SD_CLK_MODE(num) : GET_SD_DATA_MODE(num);
                #endif
                    ithGpioSetMode(num, mode);
                    //ithGpioCtrlEnable(num, ITH_GPIO_PULL_ENABLE);      /* sd io pull up */
                    //ithGpioCtrlEnable(num, ITH_GPIO_PULL_UP);
                    ithGpioSetDriving(num, ITH_GPIO_DRIVING_1);
                }
                if ((i != 0) && (num != (unsigned char)-1)) {
                #if (CFG_CHIP_FAMILY == 9860)
                    if ((i == 9) && (num == 52)) {
                        sd_pin_sel |= (GET_SDC_52(num) == 1) ? 0 : (1 << GET_SD_PIN_INDEX_52(num));
                    }
                    else {
                        sd_pin_sel |= (GET_SDC(num) == 1) ? 0 : (1 << GET_SD_PIN_INDEX(num));
                    }
                #else
                    sd_pin_sel |= (GET_SDC(num) == 1) ? 0 : (1 << GET_SD_PIN_INDEX(num));
                #endif
                }
            }
            ithWriteRegA(ITH_SDG_BASE + 0x0, sd_pin_sel);

            #if defined(CFG_SD1_NO_PIN_SHARE)
            sd1_select = true;
            #endif
    }
        break;
    case ITH_STOR_NOR:
    #ifdef CFG_SPI0_40MHZ_ENABLE
        ithWriteRegMaskA(ITH_SSP0_BASE + 0x74, CFG_SPI0_NORCLK_PARA << 16, (0xFF << 16));
    #endif
        break;
    case ITH_STOR_NAND:
    #ifdef CFG_SPI0_40MHZ_ENABLE
        ithWriteRegMaskA(ITH_SSP0_BASE + 0x74, CFG_SPI0_NANDCLK_PARA << 16, (0xFF << 16));
    #endif
        break;
    default:
        break;
    }

#if defined(CFG_SD0_NO_PIN_SHARE) || defined(CFG_SD1_NO_PIN_SHARE)
end:
#endif
    return;
}


void ithSdPowerReset(ITHStorage storage)
{
    unsigned int gpio_num;

    switch (storage)
    {
    case ITH_STOR_SD:
        gpio_num = cardCfg.powerEnablePins[ITH_CARDPIN_SD0];
        if (gpio_num == (unsigned char)-1) {
            goto end;
        }
        ithStorageUnSelect(ITH_STOR_SD);
        ithCardPowerOff(ITH_CARDPIN_SD0);
        (void)usleep(30 * 1000);
        ithCardPowerOn(ITH_CARDPIN_SD0);
        (void)usleep(35 * 1000);
        break;
    case ITH_STOR_SD1:
        gpio_num = cardCfg.powerEnablePins[ITH_CARDPIN_SD1];
        if (gpio_num == (unsigned char)-1) {
            goto end;
        }
        ithStorageUnSelect(ITH_STOR_SD1);
        ithCardPowerOff(ITH_CARDPIN_SD1);
        (void)usleep(30 * 1000);
        ithCardPowerOn(ITH_CARDPIN_SD1);
        (void)usleep(35 * 1000);
        break;
    default:
        break;
    }

end:
    return;
}

bool ithSd4bitMode(ITHStorage storage)
{
    bool is_4bit = true;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_SD0_CARD_1BIT)
        is_4bit = false;
        #endif
        break;
    case ITH_STOR_SD1:
        #if defined(CFG_SD1_CARD_1BIT)
        is_4bit = false;
        #endif
        break;
    default:
        break;
    }

    return is_4bit;
}

bool ithSdio4bitMode(void)
{
    bool is_4bit;

#if defined(CFG_SDIO_4BIT_MODE)
    is_4bit = true;
#else
    is_4bit = false;
#endif

    return is_4bit;
}

bool ithSdioLenientFn0(void)
{
    return true;
}

static int sdio_irq_thread_priority = 4;

void ithSdioSetPriority(int priority)
{
    sdio_irq_thread_priority = priority;
}

int ithSdioGetPriority(void)
{
    return sdio_irq_thread_priority;
}

static int sd_tasklet_thread_priority = 4;

void ithSdSetTaskletPriority(int priority)
{
    sd_tasklet_thread_priority = priority;
}

int ithSdGetTaskletPriority(void)
{
    return sd_tasklet_thread_priority;
}

uint32_t ithSdMaxClk(ITHStorage storage)
{
    uint32_t clock;

    switch (storage)
    {
    case ITH_STOR_SD:
        clock = 50000000;
        break;
    case ITH_STOR_SD1:
        clock = 50000000;
        break;
    default:
        clock = 300000;
        break;
    }

    return clock;
}

bool ithSdNoPinShare(ITHStorage storage)
{
    bool no_pin_share = false;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_SD0_NO_PIN_SHARE)
        no_pin_share = true;
        #endif
        break;
    case ITH_STOR_SD1:
        #if defined(CFG_SD1_NO_PIN_SHARE)
        no_pin_share = true;
        #endif
        break;
    default:
        break;
    }

    return no_pin_share;
}

bool ithSdIsEmmc(ITHStorage storage)
{
    bool is_emmc = false;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_SD0_CARD_EMMC)
        is_emmc = true;
        #endif
        break;
    case ITH_STOR_SD1:
        #if defined(CFG_SD1_CARD_EMMC)
        is_emmc = true;
        #endif
        break;
    default:
        break;
    }

    return is_emmc;
}

bool ithSdIsSdio(ITHStorage storage)
{
    bool is_sdio = false;

    switch (storage)
    {
    case ITH_STOR_SD:
#if defined(CFG_SDIO0_STATIC)
        is_sdio = true;
#endif
        break;
    case ITH_STOR_SD1:
#if defined(CFG_SDIO1_STATIC)
        is_sdio = true;
#endif
        break;
    default:
        break;
    }

    return is_sdio;
}

bool ithSdIs18V(ITHStorage storage)
{
    bool is_18v = false;

    switch (storage)
    {
    case ITH_STOR_SD:
#if defined(CFG_SD0_18V)
        is_18v = true;
#endif
        break;
    case ITH_STOR_SD1:
#if defined(CFG_SD1_18V)
        is_18v = true;
#endif
        break;
    default:
        break;
    }

    return is_18v;
}

#define EXT_CSD_MANUAL_BKOPS    1
#define EXT_CSD_AUTO_BKOPS      2

uint8_t ithEmmcGetBkopsMode(ITHStorage storage)
{
    uint8_t mode = 0;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_EMMC0_BKOPS_MANUAL)
        mode = EXT_CSD_MANUAL_BKOPS;
        #endif
        #if defined(CFG_EMMC0_BKOPS_AUTO)
        mode = EXT_CSD_AUTO_BKOPS;
        #endif
        break;

    case ITH_STOR_SD1:
        #if defined(CFG_EMMC1_BKOPS_MANUAL)
        mode = EXT_CSD_MANUAL_BKOPS;
        #endif
        #if defined(CFG_EMMC1_BKOPS_AUTO)
        mode = EXT_CSD_AUTO_BKOPS;
        #endif
        break;

    default:
        break;
    }

    return mode;
}

bool ithEmmcInitForceDoBkops(ITHStorage storage)
{
    bool do_init_bkops = false;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_EMMC0_INIT_FORCE_DO_BKOPS)
        do_init_bkops = true;
        #endif
        break;
    case ITH_STOR_SD1:
        #if defined(CFG_EMMC1_INIT_FORCE_DO_BKOPS)
        do_init_bkops = true;
        #endif
        break;
    default:
        break;
    }

    return do_init_bkops;
}

bool ithEmmcIsCacheEnable(ITHStorage storage)
{
    bool enable = false;

    switch (storage)
    {
    case ITH_STOR_SD:
#if defined(CFG_EMMC0_CACHE_ENABLE)
        enable = true;
#endif
        break;
    case ITH_STOR_SD1:
#if defined(CFG_EMMC1_CACHE_ENABLE)
        enable = true;
#endif
        break;
    default:
        break;
    }

    return enable;
}

bool ithEmmcPowerOffNotifyEn(ITHStorage storage)
{
    bool enable = false;

    switch (storage)
    {
    case ITH_STOR_SD:
        #if defined(CFG_EMMC0_POWER_OFF_NOTIFY_ENABLE)
        enable = true;
        #endif
        break;
    case ITH_STOR_SD1:
        #if defined(CFG_EMMC1_POWER_OFF_NOTIFY_ENABLE)
        enable = true;
        #endif
        break;
    default:
        break;
    }

    return enable;
}


#define SD_DELAY_REG     (ITH_SDG_BASE + 0x4)

static uint32_t sd0_ic_delay_val = 6;
static uint32_t sd0_ip_delay_val = 0x00000101;
static uint32_t sd1_ic_delay_val = 6;
static uint32_t sd1_ip_delay_val = 0x00000101;

void ithSdSetDelay(ITHStorage storage, uint32_t ic_delay, uint32_t ip_delay)
{
    switch (storage)
    {
    case ITH_STOR_SD:
        sd0_ic_delay_val = ic_delay;
        sd0_ip_delay_val = ip_delay;
        break;
    case ITH_STOR_SD1:
        sd1_ic_delay_val = ic_delay;
        sd1_ip_delay_val = ip_delay;
        break;
    default:
        sd0_ic_delay_val = ic_delay;
        sd0_ip_delay_val = ip_delay;
        break;
    }

    return;
}

uint32_t ithSdDelay(ITHStorage storage)
{
    uint32_t delay_offset = (storage == ITH_STOR_SD) ? 0 : 4;
    uint32_t delay_val;

    switch (storage)
    {
    case ITH_STOR_SD:
        ithWriteRegMaskA(SD_DELAY_REG, (sd0_ic_delay_val << delay_offset), (0xF << delay_offset));
        delay_val = sd0_ip_delay_val;
        break;
    case ITH_STOR_SD1:
        ithWriteRegMaskA(SD_DELAY_REG, (sd1_ic_delay_val << delay_offset), (0xF << delay_offset));
        delay_val = sd1_ip_delay_val;
        break;
    default:
        delay_val = sd0_ip_delay_val;
        break;
    }

    return delay_val;
}

void ithSdClockGating(ITHStorage storage)
{
#if defined(CFG_SD0_SW_SCLK_GATING) || defined(CFG_SD1_SW_SCLK_GATING)
    unsigned int num;
#endif

    switch (storage)
    {
    case ITH_STOR_SD:
#if defined(CFG_SD0_SW_SCLK_GATING)
        num = cardCfg.sd0Pins[0];
        if (num != (unsigned char)-1) 
        {
            ithGpioSetMode(num, ITH_GPIO_MODE0);
#if defined(CFG_SD0_NO_PIN_SHARE)
            sd0_select = false;
#endif
        }
#endif // #if defined(CFG_SD0_SW_SCLK_GATING)
        break;
    case ITH_STOR_SD1:
#if defined(CFG_SD1_SW_SCLK_GATING)
        num = cardCfg.sd1Pins[0];
        if (num != (unsigned char)-1) 
        {
            ithGpioSetMode(num, ITH_GPIO_MODE0);
#if defined(CFG_SD1_NO_PIN_SHARE)
            sd1_select = false;
#endif
        }
#endif // #if defined(CFG_SD1_SW_SCLK_GATING)
        break;
    default:
        break;
    }

    return;
}

void ithSdSetGpioLow(ITHStorage storage)
{
    int           i = 0;
    unsigned int num;

#if defined(CFG_SDIO1_STATIC) && !defined(CFG_SD1_NO_PIN_SHARE) && defined(CFG_SD0_ENABLE)
    /* disable sd0 clock */
    num = cardCfg.sd0Pins[0];
    if (num != (unsigned char)-1) {
        ithGpioSetMode(num, ITH_GPIO_MODE0);
    }
#endif

#if defined(CFG_SDIO0_STATIC) && !defined(CFG_SD0_NO_PIN_SHARE) && defined(CFG_SD1_ENABLE)
    /* disable sd1 clock */
    num = cardCfg.sd1Pins[0];
    if (num != (unsigned char)-1) {
        ithGpioSetMode(num, ITH_GPIO_MODE0);
    }
#endif

    switch (storage)
    {
    case ITH_STOR_SD:
        for (i = 0; i < SD_PIN_NUM; i++)
        {
            num = cardCfg.sd0Pins[i];
            if (num != (unsigned char)-1)
            {
                ithGpioSetMode(num, ITH_GPIO_MODE0);
                ithGpioClear(num);
                ithGpioSetOut(num);
                ithGpioEnable(num);
            }
        }
        break;
    case ITH_STOR_SD1:
        for (i = 0; i < SD_PIN_NUM; i++)
        {
            num = cardCfg.sd1Pins[i];
            if (num != (unsigned char)-1)
            {
                ithGpioSetMode(num, ITH_GPIO_MODE0);
                ithGpioClear(num);
                ithGpioSetOut(num);
                ithGpioEnable(num);
            }
        }
        break;
    default:
        break;
    }

    return;
}

