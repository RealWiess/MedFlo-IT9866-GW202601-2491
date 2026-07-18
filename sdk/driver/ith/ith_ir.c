/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL IR functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"

static unsigned int irWidth;

static int _ithIrPrescaleCheck(int busClk, unsigned int precision)
{
    if (busClk <= precision)
    {
        ithPrintf("busClk(%d) must be greater than precision(%d). Return!\n", busClk, precision);
        goto err;
    }
    if (busClk % precision)
    {
        ithPrintf("busClk(%d) is not divisible by precision(%d).\n", busClk, precision);
        ithPrintf("It won't be accurate enough in any operation. Return!\n");
        goto err;
    }

    return 0;
err:
    return 1;
}

static void ithIrTxSetGpio(ITHIrPort port, unsigned int pin)
{
    ithWriteRegMaskA(port + ITH_IR_GPIO_SEL_REG, pin << 8, 0x7F << 8);
    ithSetRegBitA(port + ITH_IR_GPIO_SEL_REG, 15);  //Enable TX GPIO Select
}

void ithIrRxInit(ITHIrPort port, int pin, unsigned long extClk, unsigned int precision)
{
    unsigned int busClk, prescale;

    if (extClk == 0)
    {
        busClk = ithGetBusClock();
    }
    else
    {
        unsigned int outdiv = (ithReadRegA(ITH_APB_CLK1_REG) & ITH_WCLK_RATIO_MASK) + 4;
        busClk = (extClk / outdiv) << 2;
    }

    if (_ithIrPrescaleCheck(busClk, precision))
        return;

    ithGpioSetMode(pin, ITH_GPIO_MODE0);
    ithIrRxSetGpio(port, pin);
    ithIrRxSetCaptureEdge(port, ITH_IR_BOTH);
    ithIrRxCtrlEnable(port, ITH_IR_DEBOUNCE);
    ithIrRxCtrlEnable(port, ITH_IR_SIGINVESE);
    ithIrRxCtrlEnable(port, ITH_IR_TMRST);
    ithIrRxCtrlEnable(port, ITH_IR_WARP);
    ithIrRxClear(port);

    prescale = busClk / precision - 1;

    ithWriteRegA(port + ITH_IR_RX_PRESCALE_REG, prescale);
    irWidth = (ithReadRegA(port + ITH_IR_HWCFG_REG) & ITH_IR_WIDTH_MASK) >> ITH_IR_WIDTH_BIT;
}

void ithIrTxInit(ITHIrPort port, int pin, unsigned long extClk, unsigned int precision)
{
    unsigned int busClk, prescale;

    if (extClk == 0)
    {
        busClk = ithGetBusClock();
    }
    else
    {
        unsigned int outdiv = (ithReadRegA(ITH_APB_CLK1_REG) & ITH_WCLK_RATIO_MASK) + 4;
        busClk = (extClk / outdiv) << 2;
    }

    if (_ithIrPrescaleCheck(busClk, precision))
        return;

    //IT9860 can use all pin
    ithIrTxSetGpio(port, pin);

    //ithIrTxSetCaptureMode(port, ITH_IRTX_CFG_LENGTH);
    //ithIrTxCtrlEnable(port, ITH_IR_TX_SIGINVESE);
    ithIrTxClear(port);

    //ithWriteRegMaskA(port + ITH_IRTX_CAP_CTRL_REG, 1<<1, 1<<1 );    //enable TX loop-back RX mode

    //set IR-TX clock
    prescale = busClk / precision - 1;

    ithWriteRegA(port + ITH_IR_TX_PRESCALE_REG, prescale);

    irWidth = (ithReadRegA(port + ITH_IR_HWCFG_REG) & ITH_IR_WIDTH_MASK) >> ITH_IR_WIDTH_BIT;
}

void ithIrRxDeinit(ITHIrPort port)
{
    ithIrRxSetGpio(port, (unsigned int)-1);
    ithIrRxCtrlDisable(port, ITH_IR_DEBOUNCE);
    ithIrRxCtrlDisable(port, ITH_IR_SIGINVESE);
    ithIrRxCtrlDisable(port, ITH_IR_TMRST);
    ithIrRxCtrlDisable(port, ITH_IR_WARP);
    ithIrRxClear(port);

    ithWriteRegA(port + ITH_IR_RX_PRESCALE_REG, 0);
    irWidth = 0;
}

void ithIrTxDeinit(ITHIrPort port)
{
    ithIrTxSetGpio(port, (unsigned int)-1);
    ithIrTxClear(port);

    ithWriteRegA(port + ITH_IR_TX_PRESCALE_REG, 0);
    irWidth = 0;
}

int ithIrProbe(ITHIrPort port)
{
    uint32_t status = ithReadRegA(port + ITH_IR_RX_STATUS_REG);

    if(status & (0x1 << ITH_IR_OE_BIT)){
        // ithPrintf("ithIrProbe errorrrrrrrrrrrrrrrrrrr!\n");
        ITH_LOG_WARN("IR overrun error: 0x%X\n that may loss some data.", status );
        return -1;
    }

    if (status & (0x1 << ITH_IR_DATAREADY_BIT))
        return ithReadRegA(port + ITH_IR_RX_DATA_REG) & ((0x1 << irWidth) - 1);
    else
    {
        if(status & (0x1 << ITH_IR_OE_BIT))
            ITH_LOG_WARN("IR overrun error: 0x%X\n", status );

        return -1;
    }
}

void ithClkSampleInit(ITHIrPort port, int pin, unsigned long extClk, unsigned int precision)
{
    unsigned int busClk, prescale;

    if (extClk == 0)
    {
        busClk = ithGetBusClock();
    }
    else
    {
        unsigned int outdiv = (ithReadRegA(ITH_APB_CLK1_REG) & ITH_WCLK_RATIO_MASK) + 4;
        busClk = (extClk / outdiv) << 2;
    }

    if (_ithIrPrescaleCheck(busClk, precision))
        return;

    prescale = busClk / precision - 1;

    //Reset APB IR clock
    //ithSetRegBitA(ITH_HOST_BASE + ITH_APB_CLK2_REG, ITH_IR0_RESET_BIT + ((port - ITH_IR0) >> 20 & 3));

    //Set preScale
    ithWriteRegA(port + ITH_IR_RX_PRESCALE_REG, prescale);

    //Set Gpio
    ithIrRxSetGpio(port, pin);
}
