/***************************************************************************
 *
 *            Copyright (c) 2010-2019 by HCC Embedded
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
#ifndef _CONFIG_USBH_CDCACM_H
#define _CONFIG_USBH_CDCACM_H

#include "../version/ver_usbh_cdcacm.h"
#if VER_USBH_CDCACM_MAJOR != 2 || VER_USBH_CDCACM_MINOR != 13
 #error Incompatible USBH_CDCACM version number!
#endif

#if 1 // Irene Lin
#define USBH_CDCACM_MAX_UNITS               2           /* number of maximum CDC serial units */ 
#else
#define USBH_CDCACM_MAX_UNITS               1           /* number of maximum CDC serial units */ 
#endif
#define USBH_CDCACM_MAX_UNITS_PER_INTERFACE 1           /* number of maximum data interfaces/interface */
#define USBH_CDCACM_RXBUF_COUNT             2           /* number of buffers to use for receive */
#define USBH_CDCACM_RXBUF_SIZE              512         /* size of a receive buffer (must be multiple of 64 */
                                                        /* for FS and 512 for HS controllers) */

#define USBH_CDCACM_COM_INTERFACE_ENABLE    1           /* enable COM interface usage */
#define USBH_CDCACM_COMBUF_SIZE             64          /* size of comm. interface buffer */

#define USBH_CDCACM_ALT0_DATA_CLASS         0xff        /* CDC ACM alternate 0 data class */
#define USBH_CDCACM_ALT0_DATA_SCLASS        0x00
#define USBH_CDCACM_ALT0_DATA_PROTOCOL      0x00

#define USBH_CDCACM_ALT1_DATA_CLASS         0xff        /* CDC ACM alternate 1 data class */
#define USBH_CDCACM_ALT1_DATA_SCLASS        0xff
#define USBH_CDCACM_ALT1_DATA_PROTOCOL      0xff

#endif /* ifndef _CONFIG_USBH_CDCACM_H */


