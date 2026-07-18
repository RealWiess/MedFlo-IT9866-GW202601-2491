#ifndef ITH_USB_H
#define ITH_USB_H

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_usb USB
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * USB module definition.
 */
typedef enum
{
    ITH_USB0    = ITH_USB0_BASE, ///< USB #0
    #if (CFG_CHIP_FAMILY == 970)
    ITH_USB1    = ITH_USB1_BASE  ///< USB #1
    #endif
} ITHUsbModule;

/**
 * Enters suspend mode on specified USB module.
 *
 * @param usb The USB module to suspend.
 */
void ithUsbSuspend(ITHUsbModule usb);

/**
 * Resumes from suspend mode on specified USB module.
 *
 * @param usb The USB module to resume.
 */
void ithUsbResume(ITHUsbModule usb);

/**
 * Enables the clock of USB module.
 */
void ithUsbEnableClock(void);

/**
 * Disables the clock of USB module.
 */
void ithUsbDisableClock(void);

/**
 * Reset the USB module.
 */
void ithUsbReset(void);

/**
 * USB AHB master interface type.
 */
typedef enum
{
    ITH_USB_WRAP    = 0, ///< Select USBWrap
    ITH_USB_AMBA    = 1  ///< Select AMBA
} ITHUsbInterface;

/**
 * USB AHB master interface select.
 * 
 * @param intf
 */
void ithUsbInterfaceSel(ITHUsbInterface intf);

/**
 * USB PHY power on.
 *
 * @param usb   ITH_USB0 or ITH_USB1
 *
 * - Example:
 * @code
 * ithUsbPhyPowerOn(ITH_USB0);
 * @endcode
 */
void ithUsbPhyPowerOn(ITHUsbModule usb);

/**
 * USB PHY clock ready or not.
 *
 * @param usb   ITH_USB0 or ITH_USB1
 *
 * - Example:
 * @code
 * ithUsbPhyClockReady(ITH_USB0);
 * @endcode
 */
int ithUsbPhyClockReady(ITHUsbModule usb);

/**
 * USB Device is present or not.
 *
 * @param usb   ITH_USB0 or ITH_USB1
 *
 * - Example:
 * @code
 * ithUsbDevicePresent(ITH_USB0);
 * @endcode
 */
int ithUsbDevicePresent(ITHUsbModule usb);

/**
 * Force USB Device mode.
 *
 * @param usb   ITH_USB0 or ITH_USB1
 * @param enable   enable or not
 *
 * - Example:
 * @code
 * ithUsbForceDeviceMode(ITH_USB0, true);
 * @endcode
 */
void ithUsbForceDeviceMode(ITHUsbModule usb, bool enable);

/**
* Set USB ID pin.
*
* @param usb   ITH_USB0 or ITH_USB1
* @param id_pin   the gpio number for ID pin
*
* - Example:
* @code
* ithUsbSetIdPin(ITH_USB0, 8);
* @endcode
*/
void ithUsbSetIdPin(ITHUsbModule usb, uint32_t id_pin);

/**
* Trigger Remote Wakeup.
*
* @param usb   ITH_USB0 or ITH_USB1
*
* - Example:
* @code
* ithUsbSetRemoteWakeup(ITH_USB0);
* @endcode
*/
void ithUsbSetRemoteWakeup(ITHUsbModule usb);

/**
 * The USB is in device mode or not.
 *
 * @param usb   ITH_USB0 or ITH_USB1
 * @param enable   enable or not
 *
 * - Example:
 * @code
 * ithUsbIsDeviceMode(ITH_USB0);
 * @endcode
 */
bool ithUsbIsDeviceMode(ITHUsbModule usb);

/**
 * USB wrap flush.
 */
void ithUsbWrapFlush(void);

/**
 * Enables the clock of USB's PHY module.
 * 
 * @param usb
 */
static inline void ithUsbPhyEnableClock(ITHUsbModule usb)
{
#if (CFG_CHIP_FAMILY == 9860)  
    ithWriteRegMaskA(usb + 0x3C, 0x2, 0x2);
#else
    ithWriteRegMaskA(usb + 0x58, 0x2, 0x2);
#endif
}


/**
 * Disables the clock of USB's PHY module.
 * 
 * @param usb
 */
static inline void ithUsbPhyDisableClock(ITHUsbModule usb)
{
#if (CFG_CHIP_FAMILY == 9860)  
    ithWriteRegMaskA(usb + 0x3C, 0x0, 0x2);
#else
    ithWriteRegMaskA(usb + 0x58, 0x0, 0x2);
#endif
}


#ifdef __cplusplus
}
#endif

#endif // ITH_USB_H
/** @} */ // end of ith_usb
/** @} */ // end of ith
