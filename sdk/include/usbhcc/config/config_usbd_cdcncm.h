/****************************************************************************
 *
 *            Copyright (c) 2021 by HCC Embedded
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
#ifndef CONFIG_USBD_CDCNCM_H
#define CONFIG_USBD_CDCNCM_H

#include "../version/ver_usbd_cdcncm.h"
#if VER_USBD_CDCNCM_MAJOR != 1 || VER_USBD_CDCNCM_MINOR != 2
 #error Incompatible USBD_CDCNCM verion number
#endif


#define CDCNCM_NTB_IN_MAX_SIZE          2048u /* maximum size for NTB IN transfers ( device -> host ) */
#define CDCNCM_NTB_OUT_MAX_SIZE         2048u /* maximum size for NTB OUT transfers ( host -> device ) */

/* sent in ConnectionSpeedChange notification before NetworkConnection notification */
#define CDCNCM_CONNECTION_SPEED_BITRATE 1000000u

#endif /* ifndef CONFIG_USBD_CDCNCM_H */

