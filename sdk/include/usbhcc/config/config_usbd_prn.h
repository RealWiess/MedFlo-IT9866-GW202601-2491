/***************************************************************************
 *
 *            Copyright (c) 2011-2021 by HCC Embedded
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
#ifndef _CONFIG_USBD_PRN_H_
#define _CONFIG_USBD_PRN_H_

#include "../version/ver_usbd_prn.h"
#if VER_USBD_PRN_MAJOR != 2 || VER_USBD_PRN_MINOR != 4
 #error "Invalid USBD_PRN version"
#endif

/* IEE1284 device ID. */
#define USBD_PRN_DEVICE_ID_STRING      "\x28\0MFG:HCC-Embedded Kft;CMD:PCL;MODEL:USB printer demo"

/* Set to one to use safe reception. Needed by Windows XP and Windows 7 SP1*/
#define USBD_PRN_USE_SAFE_RECEIVE      1


/* Size of receive buffer. If USBD_PRN_USE_SAFE_RECEIVE is set to 1 shall match the maximum
   packet sioz of the bulk out endpoint. */
#define USBD_PRN_DEVICE_RX_BUFFER_SIZE ( 64 )

#endif /* _CONFIG_USBD_PRN_H_ */

