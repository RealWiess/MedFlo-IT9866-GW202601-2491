#ifndef ITH_STORAGE_H
#define ITH_STORAGE_H

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_storage Storage
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The pin of card definition.
 */
typedef enum
{
    ITH_CARDPIN_SD0 = 0,    ///< SD0 card pin
    ITH_CARDPIN_SD1,        ///< SD1 card pin
    ITH_CARDPIN_MAX
} ITHCardPin;

#define SD_PIN_NUM 10
/**
 * The card configuration definition.
 */
typedef struct
{
    uint8_t cardDetectPins[ITH_CARDPIN_MAX];    ///< Array of card detection GPIO pins
    uint8_t powerEnablePins[ITH_CARDPIN_MAX];   ///< Array of power enable GPIO pins
    uint8_t writeProtectPins[ITH_CARDPIN_MAX];  ///< Array of write protect GPIO pins
    uint8_t sd0Pins[SD_PIN_NUM];
    uint8_t sd1Pins[SD_PIN_NUM];
} ITHCardConfig;

/**
 * Initializes card module.
 *
 * @param cfg The card configuration to set.
 */
void ithCardInit(const ITHCardConfig *cfg);

/**
 * Power on specified card.
 *
 * @param pin The card power on pin to power on.
 */
void ithCardPowerOn(ITHCardPin pin);

/**
 * Power off specified card.
 *
 * @param pin The card power on pin to power off.
 */
void ithCardPowerOff(ITHCardPin pin);

#if defined(CFG_GPIO_SD_WIFI_POWER_ENABLE) && defined(CFG_NET_WIFI_SDIO_VND_RTK)
void ithWIFICardPullHighFromPin(unsigned int pin);
void ithWIFICardPullLowFromPin(unsigned int pin);


/**
 * Power process for NGPL SDIO WIFI card.
 *
 */
void ithWIFICardPowerProcess(void);
#endif

/**
 * Whether card is inserted or not.
 *
 * @param pin The card detect pin to detect.
 */
bool ithCardInserted(ITHCardPin pin);

/**
 * Whether card is locked or not.
 *
 * @param pin The card write protect pin to detect.
 */
bool ithCardLocked(ITHCardPin pin);

typedef enum
{
    ITH_STOR_NAND = 0,
    ITH_STOR_SD   = 2,
    ITH_STOR_NOR  = 6,
    ITH_STOR_SD1  = 7
} ITHStorage;

extern void *ithStorMutex;
/**
 * Select the storagte. It will set the corresponding GPIO.
 *
 * @param storage The storage be selected.
 */
void ithStorageSelect(ITHStorage storage);
/**
 * Unselect the storagte.
 *
 * @param storage The storage be unselected.
 */
void ithStorageUnSelect(ITHStorage storage);
/**
 * Power reset for the SD card.
 *
 * @param storage The SD card be selected.
 */
void ithSdPowerReset(ITHStorage storage);
/**
 * Get the SD card operation mode.  (1: 4-bit, 0: 1-bit)
 *
 * @param storage The SD card be selected.
 */
bool ithSd4bitMode(ITHStorage storage);
/**
 * Get the SDIO card operation mode.  (1: 4-bit, 0: 1-bit)
 *
 * @param storage The SD card be selected.
 */
bool ithSdio4bitMode(void);
/**
* Allow SDIO fn0 writes outside of the vendor's CCCR range.
*
* @return 1: can write outside of the vendor's CCCR range.
*         0: can't write outside of the vendor's CCCR range.
*/
bool ithSdioLenientFn0(void);
/**
 * Set the SDIO irq thread priority.
 *
 * @param priority The priority value.
 */
void ithSdioSetPriority(int priority);
/**
 * Get the SDIO irq thread priority.
 *
 * @return the priority value.
 */
int ithSdioGetPriority(void);
/**
* Set the SDIO irq thread priority.
*
* @param priority The priority value.
*/
void ithSdSetTaskletPriority(int priority);
/**
* Get the SDIO irq thread priority.
*
* @return the priority value.
*/
int ithSdGetTaskletPriority(void);
/**
 * Get the SD/SDIO max clock.
 *
 * @param storage The SD card be selected.
 */
uint32_t ithSdMaxClk(ITHStorage storage);
/**
 * Get the SD pin share information. (1: no pin share, 0: pin share)
 *
 * @param storage The SD card be selected.
 */
bool ithSdNoPinShare(ITHStorage storage);

/**
 * Is it eMMC device. (1: eMMC device, 0: not eMMC device)
 *
 * @param storage The SD card be selected.
 */
bool ithSdIsEmmc(ITHStorage storage);

/**
* Is it SDIO device. (1: SDIO device, 0: not SDIO device)
*
* @param storage The SD card be selected.
*/
bool ithSdIsSdio(ITHStorage storage);

/**
* Is it 1.8V. (true: 1.8V  false: 3.3V)
*
* @param storage The SD card be selected.
*/
bool ithSdIs18V(ITHStorage storage);

/**
* Get eMMC BKOPS mode
*
* @param storage The SD card be selected.
*/
uint8_t ithEmmcGetBkopsMode(ITHStorage storage);
/**
* Do BKOPS when initializing. (1: force do it)
*
* @param storage The SD card be selected.
*/
bool ithEmmcInitForceDoBkops(ITHStorage storage);

/**
 * Is cache enable.
 *
 * @param storage The SD card be selected.
 *
 * @return 1: enabled.
 *         0: not enabled.
 */
bool ithEmmcIsCacheEnable(ITHStorage storage);

/**
* Is power off notification function enable.
*
* @param storage The SD card be selected.
*
* @return 1: enabled.
*         0: not enabled.
*/
bool ithEmmcPowerOffNotifyEn(ITHStorage storage);
/**
 * Set the SD default delay setting.
 *
 * @param storage The SD card be selected.
 * @param ic_delay The IC delay setting.
 * @param ip_delay The IP delay setting.
 */
 void ithSdSetDelay(ITHStorage storage, uint32_t ic_delay, uint32_t ip_delay);
/**
 * Get the SD delay setting.
 *
 * @param storage The SD card be selected.
 */
uint32_t ithSdDelay(ITHStorage storage);
/**
* Gated SD bus clock.
*
* @param storage The SD card be selected.
*/
void ithSdClockGating(ITHStorage storage);
/**
 * Switch SD's pin to GPIO mode and output low.
 *
 * @param storage The SD card be selected.
 */
void ithSdSetGpioLow(ITHStorage storage);


#ifdef __cplusplus
}
#endif

#endif // ITH_STORAGE_H

/** @} */ // end of ith_storage
/** @} */ // end of ith
