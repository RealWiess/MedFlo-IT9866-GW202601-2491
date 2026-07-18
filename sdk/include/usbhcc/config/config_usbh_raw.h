/***************************************************************************
 *
 *            Copyright (c) 2012-2018 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _CONFIG_USBH_RAW_H_
#define _CONFIG_USBH_RAW_H_

#include "../version/ver_usbh_raw.h"
#if VER_USBH_RAW_MAJOR != 4 || VER_USBH_RAW_MINOR != 2
 #error Incompatible USBH_RAW version number!
#endif

/* Maximum number of devices allowed to be running in parallel. */
#define USBH_RAW_MAX_UNITS                  2u

/* Maximum number of BULK channels per interface */
#define USBH_RAW_MAX_BULK_CHANNELS_PER_INTERFACE 2u

/* Maximum number of INT channels per interface */
#define USBH_RAW_MAX_INT_CHANNELS_PER_INTERFACE 2u

/* Class, sub-class and protocol value to identify interfaces this
   class-driver shall handle. */
#define USBH_RAW_CLASS                      0xfeu
#define USBH_RAW_SUBCLASS                   0x02u
#define USBH_RAW_PROTOCOL                   0x08u

#endif /* ifndef _CONFIG_USBH_RAW_H_ */

