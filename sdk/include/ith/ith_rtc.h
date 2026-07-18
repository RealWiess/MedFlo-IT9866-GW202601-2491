#ifndef ITH_RTC_H
#define ITH_RTC_H

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_rtc RTC
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * RTC control definition.
 */
typedef enum
{
    ITH_RTC_EN              = 0,    ///< RTC enable
    ITH_RTC_INTR_SEC        = 1,    ///< RTC auto alarm per second
    ITH_RTC_INTR_MIN        = 2,    ///< RTC auto alarm per minute
    ITH_RTC_INTR_HOUR       = 3,    ///< RTC auto alarm per hour
    ITH_RTC_INTR_DAY        = 4,    ///< RTC auto alarm per day
    ITH_RTC_ALARM_INTR      = 5,    ///< RTC alarm interrupt
    ITH_RTC_DAY_ALARM_INTR  = 6,    ///< RTC day alarm interrupt
    ITH_RTC_WEEK_ALARM_INTR = 7,    ///< RTC week alarm interrupt
    ITH_RTC_COUNTER_LOAD    = 8,    ///< RTC counter load
    ITH_RTC_PWREN_MODE      = 11,   ///< RTC power enable mode
    ITH_RTC_EXTPWRUP_EN     = 12,   ///< RTC external powerup enable
    ITH_RTC_PWREN_ALARM_SEL = 15,   ///< PWREN Alarm Type
    ITH_RTC_PWREN_CTRL1     = 16,   ///< PWREN Alarm Control
    ITH_RTC_SW_POWEROFF     = 17,   ///< Software power down (auto set zero)
    ITH_RTC_PWREN_IO_DIR    = 21,   ///< PWREN pin driving strength
    ITH_RTC_PWREN_GPIO      = 24,   ///< PWREN GPIO Data
    ITH_RTC_RESET           = 31    ///< Asynchronous reset
} ITHRtcCtrl;

/**
 * RTC interrupt definition.
 */
typedef enum
{
    ITH_RTC_SEC   = 0,      ///< Indicate that rtc_sec interrupt has occurred.
    ITH_RTC_MIN   = 1,      ///< Indicate that rtc_min interrupt has occurred.
    ITH_RTC_HOUR  = 2,      ///< Indicate that rtc_hour interrupt has occurred.
    ITH_RTC_DAY   = 3,      ///< Indicate that rtc_day interrupt has occurred.
    ITH_RTC_ALARM = 4,      ///< Indicate that rtc_alarm interrupt has occurred.

    ITH_RTC_MAX_INTR
} ITHRtcIntr;

/**
 * RTC power enable I/O selection definition.
 */
typedef enum
{
    ITH_RTC_PWREN  = 0,     ///< PWREN
    ITH_RTC_VCC_OK = 1,     ///< VCC_OK
    ITH_RTC_INTR   = 2,     ///< RTC interrupt
    ITH_RTC_GPIO   = 3      ///< GPIO
} ITHRtcPowerEnableIoSelection;

/**
 * RTC power enable I/O selection definition.
 */
typedef enum
{
    ITH_RTC_DIV_SRC_INNER_12MHZ = 0,      ///< divider source is internal 12Mhz clock
    ITH_RTC_DIV_SRC_EXT_32KHZ   = 1,      ///< divider source is external 32kHz clock
} ITHRtcClockSource;

/**
 * Enables specified RTC control.
 *
 * @param ctrl the control to enable.
 */
static inline void ithRtcCtrlEnable(ITHRtcCtrl ctrl)
{
    ithSetRegBitA(ITH_RTC_BASE + ITH_RTC_CR_REG, (uint32_t) ctrl);
}

/**
 * Disables specified RTC control.
 *
 * @param ctrl the control to disable.
 */
static inline void ithRtcCtrlDisable(ITHRtcCtrl ctrl)
{
    ithClearRegBitA(ITH_RTC_BASE + ITH_RTC_CR_REG, (uint32_t) ctrl);
}

/**
 * Clears specified RTC interrupt.
 *
 * @param intr the interrupt to clear.
 */
static inline void ithRtcClearIntr(ITHRtcIntr intr)
{
    ithClearRegBitA(ITH_RTC_BASE + ITH_RTC_INTRSTATE_REG, (uint32_t) intr);
}

/**
 * Gets the state of RTC interrupt.
 *
 * @return the state of RTC interrupt.
 */
static inline uint32_t ithRtcGetIntrState(void)
{
    return ithReadRegA(ITH_RTC_BASE + ITH_RTC_INTRSTATE_REG);
}

/**
 * Sets the RTC power enable I/O selection.
 *
 * @param sel the RTC power enable I/O selection.
 */
static inline void ithRtcSetPowerEnableIoSelection(ITHRtcPowerEnableIoSelection sel)
{
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_PWREN_IOSEL_REG, (uint32_t) sel << ITH_RTC_PWREN_IOSEL_BIT, ITH_RTC_PWREN_IOSEL_MASK);
}

/**
 * Sets the RTC external power up mode.
 *
 * @param mode the external power up mode.
 */
static inline void ithRtcSetExtPowerUpMode(uint32_t mode)
{
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_CR_REG, mode << ITH_RTC_EXTPWRUP_MODE_BIT, ITH_RTC_EXTPWRUP_MODE_MASK);
}

/**
 * Set RTC source.
 *
 * @param clkSrc The clock source.
 */
void ithRtcSetDivSrc(ITHRtcClockSource clkSrc);

/**
 * Get RTC source.
 *
 * @return the RTC clock source.
 */
uint32_t ithRtcGetDivSrc(void);

/**
 * Set External RTC source.
 *
 * @param source The clock source.
 */
void ithRtcSetExtSrc(uint32_t source);

/**
 * Initializes RTC module.
 *
 * @param extClk The frequency of external clock.
 */
void ithRtcInit(uint32_t extClk);

/**
 * Enables RTC module.
 *
 * @return First booting or not.
 */
bool ithRtcEnable(void);

/**
 * Gets RTC time.
 *
 * @return the RTC time.
 */
uint32_t ithRtcGetTime(void);

/**
 * Sets RTC time.
 *
 * @param t the RTC time.
 */
void ithRtcSetTime(uint32_t t);

/**
 * Gets RTC state.
 *
 * @return the RTC state.
 */
static inline uint32_t ithRtcGetState(void)
{
    return (uint32_t) (ithReadRegA(ITH_RTC_BASE + ITH_RTC_STATE_REG) & ITH_RTC_STATE_MASK) >> ITH_RTC_STATE_BIT;
}

/**
 * Sets RTC state.
 *
 * @return the RTC state.
 */
static inline void ithRtcSetState(uint32_t state)
{
    ithWriteRegMaskA(ITH_RTC_BASE + ITH_RTC_STATE_REG, state << ITH_RTC_STATE_BIT, ITH_RTC_STATE_MASK);
}

/**
 * Get RTC alarm time.
 *
 * @return the RTC alarm time.
 */
uint32_t ithRtcGetAlarm(void);

/**
 * Sets RTC alarm time.
 *
 * @param t the RTC time.
 */
void ithRtcSetAlarm(uint32_t t);

#ifdef __cplusplus
}
#endif

#endif // ITH_RTC_H
/** @} */ // end of ith_rtc
/** @} */ // end of ith