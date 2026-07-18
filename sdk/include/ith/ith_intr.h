#ifndef ITH_INTR_H
#define ITH_INTR_H

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_intr Interrupt
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interrupt number definition.
 */
typedef enum
{
    ITH_INTR_SW0        = 0,       ///< Software (interrupt is invoking when write the software interrupt to 1)
    ITH_INTR_MC         = 1,       ///< Memory Controller
    ITH_INTR_DMA        = 2,       ///< DMA / DMA Terminal Count /DMA Error
    ITH_INTR_USB0       = 3,       ///< USB0
    ITH_INTR_TIMER      = 4,       ///< Timer (for All)
    ITH_INTR_TIMER1     = 5,       ///< Timer 1
    ITH_INTR_TIMER2     = 6,       ///< Timer 2
    ITH_INTR_TIMER3     = 7,       ///< Timer 3
    ITH_INTR_TIMER4     = 8,       ///< Timer 4
    ITH_INTR_TIMER5     = 9,       ///< Timer 5
    ITH_INTR_TIMER6     = 10,      ///< Timer 6
    ITH_INTR_TIMER7     = 11,      ///< Timer 7
    ITH_INTR_TIMER8     = 12,      ///< Timer 8
    ITH_INTR_TIMER9     = 13,      ///< Timer 9
    ITH_INTR_RTCSEC     = 14,      ///< Real time clock second
    ITH_INTR_RTC        = 15,      ///< Real time clock
    ITH_INTR_RTCALARM   = 16,      ///< Real time alarm
    ITH_INTR_IR2RX      = 17,      ///< Remote IR2 RX
    ITH_INTR_IR2TX      = 18,      ///< Remote IR2 TX
    ITH_INTR_IR3RX      = 19,      ///< Remote IR3 RX
    ITH_INTR_IR3TX      = 20,      ///< Remote IR3 TX
    ITH_INTR_WD         = 21,      ///< Watch dog
    ITH_INTR_ARMDMA     = 22,      ///< ARMDMA
    ITH_INTR_ALTCPU     = 23,      ///< ALTCPU
    ITH_INTR_CMDQ       = 24,      ///< Command Queue 0
    ITH_INTR_CMDQ1      = 25,      ///< Command Queue 1
    ITH_INTR_2D         = 26,      ///< 2D
    ITH_INTR_GPIO       = 27,      ///< GPIO
    ITH_INTR_I2C0       = 28,      ///< I2C0
    ITH_INTR_I2C1       = 29,      ///< I2C1
    ITH_INTR_I2C2       = 30,      ///< I2C2
    ITH_INTR_I2C3       = 31,      ///< I2C2

    ITH_INTR_SW1        = 32 + 0,  ///< Software (interrupt is invoking when write the software interrupt register to 1) // 0
    ITH_INTR_CANBUS0    = 32 + 1,  ///< Canbus 0
    ITH_INTR_CANBUS1    = 32 + 2,  ///< Canbus 1
    ITH_INTR_MAC        = 32 + 3,  ///< MAC / MAC WOL
    ITH_INTR_WIEGAND0   = 32 + 4,  ///< Wiegand0
    ITH_INTR_WIEGAND1   = 32 + 5,  ///< Wiegand1
    ITH_INTR_UART0      = 32 + 6,  ///< UART 0
    ITH_INTR_UART1      = 32 + 7,  ///< UART 1
    ITH_INTR_UART2      = 32 + 8,  ///< UART 2
    ITH_INTR_UART3      = 32 + 9,  ///< UART 3
    ITH_INTR_UART4      = 32 + 10, ///< UART 4
    ITH_INTR_UART5      = 32 + 11, ///< UART 5
    ITH_INTR_JPEG       = 32 + 12, ///< Jpeg
    ITH_INTR_LCD        = 32 + 13, ///< LCD
    //ITH_INTR_ISP        = 32 + 14,     ///< ISP
    //ITH_INTR_IQ                    = 32 + 15,  ///< IQ
    ITH_INTR_ISP_CORE0  = 32 + 14,  ///< ISP Core 0
    ITH_INTR_ISP_CORE1  = 32 + 15,  ///< ISP Core 1
    ITH_INTR_I2S        = 32 + 16,  ///< I2S
    ITH_INTR_DPU        = 32 + 17,  ///< DPU
    ITH_INTR_DECOMPRESS = 32 + 18,  ///< Decompress
    ITH_INTR_CAPTURE    = 32 + 19,  ///< CCIR601/656 Capture
    ITH_INTR_SARADC     = 32 + 20,  ///< SARADC
    ITH_INTR_VIDEO      = 32 + 21,  ///< Video
    ITH_INTR_SD0        = 32 + 22,  ///< SD/MMC 0
    ITH_INTR_SD1        = 32 + 23,  ///< SD/MMC 1
    ITH_INTR_IR0RX      = 32 + 24,  ///< Remote IR0 RX
    ITH_INTR_IR0TX      = 32 + 25,  ///< Remote IR0 TX
    ITH_INTR_IR1RX      = 32 + 26,  ///< Remote IR1 RX
    ITH_INTR_IR1TX      = 32 + 27,  ///< Remote IR1 TX
    ITH_INTR_SSP0       = 32 + 28,  ///< SSP 0
    ITH_INTR_SSP1       = 32 + 29,  ///< SSP 1
    ITH_INTR_AXISPI     = 32 + 30,  ///< AXISPI
    ITH_INTR_MIPI       = 32 + 31   ///< MIPI   
} ITHIntr;


/**
 * Interrupt trigger mode definition.
 */
typedef enum
{
    ITH_INTR_LEVEL = 0,  ///< Level-trigger mode
    ITH_INTR_EDGE  = 1   ///< Edge-trigger mode
} ITHIntrTriggerMode;

typedef enum
{
    ITH_INTR_HIGH_RISING = 0,       ///< Active-high level trigger or rising-edge trigger
    ITH_INTR_LOW_FALLING = 1        ///< Active-low level trigger or falling-edge trigger
} ITHIntrTriggerLevel;

/**
 * Interrupt handler.
 *
 * @arg Custom argument.
 */
typedef void (*ITHIntrHandler)(void *arg);

/**
 * Initializes interrupt module.
 */
void ithIntrInit(void);

/**
 * Resets interrupt module.
 */
void ithIntrReset(void);

/**
 * Enables specified IRQ.
 *
 * @param intr The IRQ.
 */
static inline void ithIntrEnableIrq(ITHIntr intr)
{
    ithEnterCritical();

    if (intr < 32)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ1_EN_REG, intr);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ2_EN_REG, intr - 32);

    ithExitCritical();
}

/**
 * Disables specified IRQ.
 *
 * @param intr The IRQ.
 */
static inline void ithIntrDisableIrq(ITHIntr intr)
{
    ithEnterCritical();

    if (intr < 32)
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ1_EN_REG, intr);
    else
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ2_EN_REG, intr - 32);

    ithExitCritical();
}

/**
 * Clears specified IRQ.
 *
 * @param intr The IRQ.
 */
static inline void ithIntrClearIrq(ITHIntr intr)
{
    if (intr < 32)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ1_CLR_REG, intr);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ2_CLR_REG, intr - 32);
}

/**
 * Gets status of IRQ.
 *
 * @param intr1 The IRQ status 1.
 * @param intr2 The IRQ status 2.
 */
static inline void ithIntrGetStatusIrq(uint32_t *intr1, uint32_t *intr2)
{
    *intr1 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_IRQ1_STATUS_REG);
    *intr2 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_IRQ2_STATUS_REG);
}

/**
 * Sets trigger mode of IRQ.
 *
 * @param intr The IRQ.
 * @param mode The trigger mode.
 */
void ithIntrSetTriggerModeIrq(ITHIntr intr, ITHIntrTriggerMode mode);

/**
 * Sets trigger level of IRQ.
 *
 * @param intr The IRQ.
 * @param level The trigger level.
 */
void ithIntrSetTriggerLevelIrq(ITHIntr intr, ITHIntrTriggerLevel level);

/**
 * Registers IRQ handler.
 *
 * @param intr The IRQ.
 * @param handler The callback function.
 * @param arg Custom argument to pass to handler.
 */
void ithIntrRegisterHandlerIrq(ITHIntr intr, ITHIntrHandler handler, void *arg);

/**
 * Sets software IRQ.
 *
 * @param num The IRQ0 or IRQ1.
 */
static inline void ithIntrSetSwIrq(int num)
{
    if (num)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ2_SWINTR_REG, ITH_INTR_SWINT_BIT);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ1_SWINTR_REG, ITH_INTR_SWINT_BIT);
}

/**
 * Clears software IRQ.
 *
 * @param num The IRQ0 or IRQ1.
 */
static inline void ithIntrClearSwIrq(int num)
{
    if (num)
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ2_SWINTR_REG, ITH_INTR_SWINT_BIT);
    else
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_IRQ1_SWINTR_REG, ITH_INTR_SWINT_BIT);
}

/**
 * Dispatches IRQ to handlers.
 */
void ithIntrDoIrq(void);

/**
 * Enables specified FIQ.
 *
 * @param intr The FIQ.
 */
static inline void ithIntrEnableFiq(ITHIntr intr)
{
    if (intr < 32)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ1_EN_REG, intr);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ2_EN_REG, intr - 32);
}

/**
 * Disables specified FIQ.
 *
 * @param intr The FIQ.
 */
static inline void ithIntrDisableFiq(ITHIntr intr)
{
    if (intr < 32)
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ1_EN_REG, intr);
    else
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ2_EN_REG, intr - 32);
}

/**
 * Clears specified FIQ.
 *
 * @param intr The FIQ.
 */
static inline void ithIntrClearFiq(ITHIntr intr)
{
    if (intr < 32)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ1_CLR_REG, intr);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ2_CLR_REG, intr - 32);
}

/**
 * Gets status of FIQ.
 *
 * @param intr1 The FIQ status 1.
 * @param intr2 The FIQ status 2.
 */
static inline void ithIntrGetStatusFiq(uint32_t *intr1, uint32_t *intr2)
{
    *intr1 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_FIQ1_STATUS_REG);
    *intr2 = ithReadRegA(ITH_INTR_BASE + ITH_INTR_FIQ2_STATUS_REG);
}

/**
 * Sets trigger mode of FIQ.
 *
 * @param intr The FIQ.
 * @param mode The trigger mode.
 */
void ithIntrSetTriggerModeFiq(ITHIntr intr, ITHIntrTriggerMode mode);

/**
 * Sets trigger level of FIQ.
 *
 * @param intr The FIQ.
 * @param level The trigger level.
 */
void ithIntrSetTriggerLevelFiq(ITHIntr intr, ITHIntrTriggerLevel level);

/**
 * Registers FIQ handler.
 *
 * @param intr The IRQ.
 * @param handler The callback function.
 * @param arg Custom argument to pass to handler.
 */
void ithIntrRegisterHandlerFiq(ITHIntr intr, ITHIntrHandler handler, void *arg);

/**
 * Sets software FIQ.
 *
 * @param num The FIQ0 or FIQ1.
 */
static inline void ithIntrSetSwFiq(int num)
{
    if (num)
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ2_SWINTR_REG, ITH_INTR_SWINT_BIT);
    else
        ithSetRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ1_SWINTR_REG, ITH_INTR_SWINT_BIT);
}

/**
 * Clears software FIQ.
 *
 * @param num The FIQ0 or FIQ1.
 */
static inline void ithIntrClearSwFiq(int num)
{
    if (num)
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ2_SWINTR_REG, ITH_INTR_SWINT_BIT);
    else
        ithClearRegBitA(ITH_INTR_BASE + ITH_INTR_FIQ1_SWINTR_REG, ITH_INTR_SWINT_BIT);
}

/**
 * Dispatches FIQ to handlers.
 */
void ithIntrDoFiq(void);

/**
 * Suspends interrupt module.
 */
void ithIntrSuspend(void);

/**
 * Resumes from suspends for interrupt module.
 */
void ithIntrResume(void);

/**
 * Print interrupt information.
 */
void ithIntrStats(void);


#ifdef __cplusplus
}
#endif

#endif // ITH_INTR_H
/** @} */ // end of ith_intr
/** @} */ // end of ith