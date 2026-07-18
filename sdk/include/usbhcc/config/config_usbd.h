/***************************************************************************
 *
 *            Copyright (c) 2008-2023 by Tuxera
 *
 * This software is copyrighted by and is the sole property of
 * Tuxera.  All rights, title, ownership, or other interests
 * in the software remain the property of Tuxera.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of Tuxera.
 *
 * Tuxera reserves the right to modify this software without notice.
 *
 * Tuxera Inc
 * Westendintie 1
 * 02160 Espoo
 * Finland
 *
 * Tel:  +358 20 764 1720
 * http: www.tuxera.com
 * email: info@tuxera.com
 *
 ***************************************************************************/
#ifndef _CONFIG_USBD_H_
#define _CONFIG_USBD_H_

#include "../psp/include/psp_types.h"
#include "config_oal_os.h"

#include "../version/ver_usbd.h"
#if ( VER_USBD_MAJOR != 3 ) || ( VER_USBD_MINOR != 32 )
 #error "Invalid USBD version."
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum number of class driver to be supported. Some class drivers
   need multiple entries. */
#define MAX_NO_OF_CDRVS          3


/* Number of endpoints to be supported. Set it to match the capabilities
   of the low-level driver. */
#define MAX_NO_OF_EP             9


/* Define this macro with value 1 if your device shall be able to execute
    remote wakeup signaling on the USB bus. */
#define USBD_REMOTE_WAKEUP       0


/* Define this macro with value 1 if your device shall be able to perform
    Test Mode Support on the USB bus.  */
#define USBD_TEST_MODE_SUPPORT   0


/* Maximum number of interfaces in the configuration. Interface ID:s shall
   start from 0 and must increase without holes. */
#define MAX_NO_OF_INTERFACES     4

/* Enable or disable SOF timer functionality. */
#define USBD_SOFTMR_SUPPORT      0

/* Enable or disable isochronous endpoint support. */
#ifdef CFG_USBD_CD_AUDIO
 #define USBD_ISOCHRONOUS_SUPPORT 1
#else
 #define USBD_ISOCHRONOUS_SUPPORT 0
#endif

/* Enable or disable handling of MS OS descriptors and string */
#define USBD_MS_OS_DSC_SUPPORT   0
#define USBD_MS_OS_VENDOR_CODE   0xBCu     /* bMS_VendorCode, must match with vendor code returned in OS string descriptor */


/* Control task stack size. */
#define USBD_CONTROL_STACK_SIZE  (2 * 1024)

/* ep0_task stack size. */
#define USBD_EP0_STACK_SIZE      (1024 + 512)  // Irene Lin


/* Add endpoints at start. Only one configuration is possible,
   SET_CONFIGURATION is answered by USB controller automatically (SH7727) */
#define USBD_ADD_EPS_AT_START    0

#define USBD_OTG_SRPHNP_SUPPORT  0         /* Set to 1 if Session Request Protocol or
                                              Host Negotiation Protocol supported */

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CONFIG_USBD_H_ */

