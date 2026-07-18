/***************************************************************************
 *
 *            Copyright (c) 2010-2021 by HCC Embedded
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
#ifndef _CONFIG_SCSI_H
#define _CONFIG_SCSI_H

#include "../version/ver_usbh_mst.h"
#if VER_USBH_MST_MAJOR != 3 || VER_USBH_MST_MINOR != 17
 #error Incompatible USBH_MST version number!
#endif

#define SCSI_MAX_UNITS                6 // 1   // Irene Lin
#define SCSI_MAX_LUN                  6 // 2   // Irene Lin
#define SCSI_SUPPORT_REMOVABLE_DEVICE 1 // 0   // Irene Lin

#if ( SCSI_SUPPORT_REMOVABLE_DEVICE != 0 )
  
 /* If Test Unit Ready command was issued to the device within the defined */
 /* range, the last result will be provided to the upper layer */
 #define SCSI_REMOVABLE_DEVICE_POLL_INTERVAL_MS 500
#endif

/* 1: Use Stop Unit command when releasing the device. */
/* Most mass storage devices cannot be restarted without physically removing */
/* and attaching the device/media. */
/* This means that f_initvolume() will fail after f_delvolume() is called */
/* without physically removing and attaching the device. */
/* 0: Only flush cache at release. */
#define SCSI_USE_START_STOP_UNIT      0

#define RETRY_COUNT                             20
#define RETRY_WAIT_MS                           100

#endif /* ifndef _CONFIG_SCSI_H */


