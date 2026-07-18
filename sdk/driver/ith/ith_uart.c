/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL UART functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"

static uint32_t *getFCRvalue(ITHUartPort port)
{
    static uint32_t gFCRvalue[6] = {0U};

    if (port == ITH_UART0)
    {
        return &gFCRvalue[0];
    }
    else if (port == ITH_UART1)
    {
        return &gFCRvalue[1];
    }
    else if (port == ITH_UART2)
    {
        return &gFCRvalue[2];
    }
    else if (port == ITH_UART3)
    {
        return &gFCRvalue[3];
    }
    else if (port == ITH_UART4)
    {
        return &gFCRvalue[4];
    }
    else
    {
        return &gFCRvalue[5];
    }
}

/**
 * Enables specified FIFO controls.
 *
 * @param ctrl the controls to enable.
 */
void ithUartFifoCtrlEnable(ITHUartPort port, ITHUartFifoCtrl ctrl)
{
    uint32_t *pValue = getFCRvalue(port);

    *pValue |= (1UL << ITH_UART_FCR_FIFO_EN_BIT);
    ithWriteRegA(port + ITH_UART_FCR_REG, *pValue);
#if 0
    ithWriteRegA(0xde600108U, 1U);
    ithWriteRegH(0x16B0U, port + ITH_UART_FCR_REG);
#endif
}

/**
 * Disables specified FIFO controls.
 *
 * @param ctrl the controls to disable.
 */
void ithUartFifoCtrlDisable(ITHUartPort port, ITHUartFifoCtrl ctrl)
{
    uint32_t *pValue = getFCRvalue(port);

    *pValue &= ~(0x1UL << ITH_UART_FCR_FIFO_EN_BIT);
    ithWriteRegA(port + ITH_UART_FCR_REG, *pValue);
}

/**
 * Sets UART TX interrupt trigger level.
 *
 * @param port The UART port
 * @param level The UART TX trigger level
 */
void ithUartSetTxTriggerLevel(
    ITHUartPort         port,
    ITHUartTriggerLevel level)
{
    uint32_t *pValue = getFCRvalue(port);

    *pValue &= ~ITH_UART_FCR_TXFIFO_TRGL_MASK;
    *pValue |= (level << ITH_UART_FCR_TXFIFO_TRGL_BIT);
    ithWriteRegA(port + ITH_UART_FCR_REG, (*pValue | (0x1UL << ITH_UART_FCR_TXFIFO_RESET_BIT)));
}

/**
 * Sets UART RX interrupt trigger level.
 *
 * @param port The UART port
 * @param level The UART RX trigger level
 */
void ithUartSetRxTriggerLevel(
    ITHUartPort         port,
    ITHUartTriggerLevel level)
{
    uint32_t *pValue = getFCRvalue(port);

    *pValue &= ~ITH_UART_FCR_RXFIFO_TRGL_MASK;
    *pValue |= (level << ITH_UART_FCR_RXFIFO_TRGL_BIT);
    ithWriteRegA(port + ITH_UART_FCR_REG, (*pValue | (0x1UL << ITH_UART_FCR_RXFIFO_RESET_BIT)));
}

void ithUartSetMode(
    ITHUartPort     port,
    ITHUartMode     mode,
    int             txPin,
    int             rxPin)
{
    int port_n = (port - ITH_UART0) >> 8;

#if (CFG_CHIP_FAMILY == 9860)
    if ((txPin >= 21) && (txPin <= 24))
    {
        (void)ithPrintf("Hit fully mux special case, tx pin: %d change to %d\n", txPin, txPin - 21);
        txPin -= 21;
    }
    if ((rxPin >= 21) && (rxPin <= 24))
    {
        (void)ithPrintf("Hit fully mux special case, rx pin: %d won't work properly.\n", rxPin);
    }

    if (((txPin >= 68) && (txPin <= 77)) ||
        ((rxPin >= 68) && (rxPin <= 77))) /* using MIPI GPIO*/
    {
        ithWriteRegMaskA(ITH_MIPI_DPHY_BASE, 0x2UL << 22U, 0x3UL << 22U); /* set MIPI DPHY R0 to TTL*/
    }
#endif

    if (txPin > -1)
    {
        ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), txPin << ITH_GPIO_URTX_BIT, ITH_GPIO_URTX_MASK);
        ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), ITH_GPIO_URTX_EN_BIT);
        ithGpioSetMode(txPin, 0);
        ithGpioSetOut(txPin);
    }
    if (rxPin > -1)
    {
        ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), rxPin << ITH_GPIO_URRX_BIT, ITH_GPIO_URRX_MASK);
        ithSetRegBitA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), ITH_GPIO_URRX_EN_BIT);
        ithGpioSetMode(rxPin, 0);
        ithGpioSetIn(rxPin);
    }

    ithWriteRegMaskA(port + ITH_UART_MDR_REG, mode, ITH_UART_MDR_MODE_SEL_MASK);
}

void ithUartClearMode(
    ITHUartPort     port,
    ITHUartMode     mode,
    int             txPin,
    int             rxPin)
{
    int port_n = (port - ITH_UART0) >> 8;

#if (CFG_CHIP_FAMILY == 9860)
    if ((txPin >= 21) && (txPin <= 24))
    {
        (void)ithPrintf("Hit fully mux special case, tx pin: %d change to %d\n", txPin, txPin - 21);
        txPin -= 21;
    }
    if ((rxPin >= 21) && (rxPin <= 24))
    {
        (void)ithPrintf("Hit fully mux special case, rx pin: %d won't work properly.\n", rxPin);
    }
    if (((txPin >= 68) && (txPin <= 77)) || ((rxPin >= 68) && (rxPin <= 77))) /* using MIPI GPIO*/
    {
        ithWriteRegMaskA(ITH_MIPI_DPHY_BASE, 0x2UL << 22U, 0x3UL << 22U); /* set MIPI DPHY R0 to TTL*/
    }
#endif

    if (txPin > -1)
    {
        ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), ITH_GPIO_URTX_EN_BIT);
        ithGpioSetMode(txPin, 0);
        ithGpioSetOut(txPin);
    }
    if (rxPin > -1)
    {
        ithClearRegBitA(ITH_GPIO_BASE + ITH_GPIO_URSEL1_REG + (port_n * 4), ITH_GPIO_URRX_EN_BIT);
        ithGpioSetMode(rxPin, 0);
        ithGpioSetOut(rxPin);
    }
}

void ithUartSetParity(
    ITHUartPort     port,
    ITHUartParity   parity,
    unsigned int    stop,
    unsigned int    len)
{
    uint32_t lcr;
    lcr = ithReadRegA(port + ITH_UART_LCR_REG) & ~ITH_UART_LCR_DLAB;

    /* Clear orignal parity setting*/
    lcr &= 0xC0U;

    switch (parity)
    {
    case ITH_UART_ODD:
        lcr |= ITH_UART_LCR_ODD;
        break;

    case ITH_UART_EVEN:
        lcr |= ITH_UART_LCR_EVEN;
        break;

    case ITH_UART_MARK:
        lcr |= ITH_UART_LCR_STICKPARITY | ITH_UART_LCR_ODD;
        break;

    case ITH_UART_SPACE:
        lcr |= ITH_UART_LCR_STICKPARITY | ITH_UART_LCR_EVEN;
        break;

    default:
        /* do nothing */
        break;
    }

    if (stop == 2)
    {
        lcr |= ITH_UART_LCR_STOP;
    }

    lcr |= len - 5;
    ithWriteRegA(port + ITH_UART_LCR_REG, lcr);
}

void ithUartSetBaudRate(ITHUartPort     port,
                        unsigned int    baud)
{
    unsigned int    totalDiv, intDiv, fDiv;
    uint32_t        lcr;
    lcr = ithReadRegA(port + ITH_UART_LCR_REG) & ~ITH_UART_LCR_DLAB;

    /* Set DLAB = 1*/
    ithWriteRegA(port + ITH_UART_LCR_REG, ITH_UART_LCR_DLAB);

    totalDiv    = ithGetBusClock() / baud;
    intDiv      = totalDiv >> 4U;
    fDiv        = totalDiv & 0xFU;

    /* Set baud rate*/
    ithWriteRegA(   port + ITH_UART_DLM_REG,    (intDiv & 0xF00U) >> 8U);
    ithWriteRegA(   port + ITH_UART_DLL_REG,    intDiv & 0xFFU);

    /* Set fraction rate*/
    ithWriteRegA(   port + ITH_UART_DLH_REG,    fDiv & 0xFU);

    ithWriteRegA(   port + ITH_UART_LCR_REG,    lcr);
}

void ithUartReset(
    ITHUartPort     port,
    unsigned int    baud,
    ITHUartParity   parity,
    unsigned int    stop,
    unsigned int    len)
{
    unsigned int    totalDiv, intDiv, fDiv;
    uint32_t        lcr;

    /* Power on clock*/
    ithSetRegBitA(ITH_APB_CLK2_REG, ITH_EN_W6CLK_BIT);

    /* Temporarily setting?*/
#if ((CFG_CHIP_FAMILY == 9070) || (CFG_CHIP_FAMILY == 9910))
    if (port == ITH_UART0)
    {
        ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_MISC_SET_REG,
                         ITH_GPIO_HOSTSEL_GPIO << ITH_GPIO_HOST_SEL_POS,
                         ITH_GPIO_HOST_SEL_MSK);
    }
#endif

    totalDiv    = ithGetBusClock() / baud;
    intDiv      = totalDiv >> 4U;
    fDiv        = totalDiv & 0xFU;

    lcr         = ithReadRegA(port + ITH_UART_LCR_REG) & ~ITH_UART_LCR_DLAB;

    /* Set DLAB = 1*/
    ithWriteRegA(   port + ITH_UART_LCR_REG,    ITH_UART_LCR_DLAB);

    /* Set baud rate*/
    ithWriteRegA(   port + ITH_UART_DLM_REG,    (intDiv & 0xFF00U) >> 8U);
    ithWriteRegA(   port + ITH_UART_DLL_REG,    intDiv & 0xFFU);

    /* Set fraction rate*/
    ithWriteRegA(   port + ITH_UART_DLH_REG,    fDiv & 0xFU);

    /* Clear orignal parity setting*/
    lcr &= 0xC0U;

    switch (parity)
    {
    case ITH_UART_ODD:
        lcr |= ITH_UART_LCR_ODD;
        break;

    case ITH_UART_EVEN:
        lcr |= ITH_UART_LCR_EVEN;
        break;

    case ITH_UART_MARK:
        lcr |= ITH_UART_LCR_STICKPARITY | ITH_UART_LCR_ODD;
        break;

    case ITH_UART_SPACE:
        lcr |= ITH_UART_LCR_STICKPARITY | ITH_UART_LCR_EVEN;
        break;

    default:
        /* do nothing */
        break;
    }

    if (stop == 2U)
    {
        lcr |= ITH_UART_LCR_STOP;
    }

    lcr |= len - 5;

    ithWriteRegA(port + ITH_UART_LCR_REG, lcr);

    ithUartFifoCtrlEnable(port, ITH_UART_FIFO_EN);  /* enable fifo as default*/
    ithUartSetTxTriggerLevel(port, ITH_UART_TRGL2); /* default to maximum level*/
    ithUartSetRxTriggerLevel(port, ITH_UART_TRGL0); /* default to maximum level*/
}
