/***************************************************************************
 *
 *            Copyright (c) 2010-2020 by HCC Embedded
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
#ifndef _CONFIG_USBH_CDCECM_H
#define _CONFIG_USBH_CDCECM_H

#include "../version/ver_usbh_cdcecm.h"
#if VER_USBH_CDCECM_MAJOR != 2 || VER_USBH_CDCECM_MINOR != 10
 #error Incompatible USBH_CDCECM version number!
#endif

#define USBH_CDCECM_MAX_UNITS    1u     /* number of maximum CDC ECM units */
#define USBH_CDCECM_EXTERNAL_BUF 0u     /* use externally provided buffer (1) or internal buffers (0) */  // Irene Lin
#if ( USBH_CDCECM_EXTERNAL_BUF == 0u )
 #define USBH_CDCECM_RXBUF_SIZE  1664u /*2048u*/  /* size of a receive buffer (must be multiple of 64 */  // Irene Lin
                                        /* for FS and 512 for HS controllers) */
 #define USBH_CDCECM_RXBUF_COUNT 32u /*2u*/     /* number of buffers to use for receive */  // Irene Lin
#endif
#define USBH_CDCECM_COMBUF_SIZE  64u    /* size of comm. interface buffer */

#endif /* ifndef _CONFIG_USBH_CDCECM_H */

