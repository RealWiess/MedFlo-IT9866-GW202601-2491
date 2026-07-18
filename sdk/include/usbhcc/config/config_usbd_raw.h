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
#ifndef _CONFIG_USBD_RAW_H_
#define _CONFIG_USBD_RAW_H_

#include "config_oal_os.h"
#include "../version/ver_usbd_raw.h"

#if VER_USBD_RAW_MAJOR != 4 || VER_USBD_RAW_MINOR != 1
 #error "Invalid USBD_RAW version."
#endif

#define USBD_RAW_CLASS         0xfe
#define USBD_RAW_SUBCLASS      0x02
#define USBD_RAW_PROTOCOL      0x08

#define USBD_RAW_USE_INT_IN    0   /* set to 1 if interrupt IN channel is used */
#define USBD_RAW_USE_INT_OUT   0   /* set to 1 if interrupt OUT channel is used */

#define USBD_RAW_ONE_INTERFACE 1   /* set if all channels are on the same interface */

#define USBD_RAW_NUM_CH        1u  /* number of channels - must match the device descriptor! */

#endif /* ifndef _CONFIG_USBD_RAW_H_ */

