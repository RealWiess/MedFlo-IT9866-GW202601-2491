/***************************************************************************
 *
 *            Copyright (c) 2010-2024 by Tuxera
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
#ifndef _CONFIG_USB_HOST_H
#define _CONFIG_USB_HOST_H


#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3 || VER_USB_HOST_MINOR != 28
 #error Incompatible USB_HOST version number!
#endif

#define USBH_PMGR_TASK_STACK_SIZE     1024
#define USBH_PMGR_REQ_TASK_STACK_SIZE (1024+1024) //1024 // Irene Lin

/* max. no. host controllers */
#if (CFG_CHIP_FAMILY == 970) // Irene Lin
#define USBH_MAX_HOST_CONTROLLERS     2
#else
#define USBH_MAX_HOST_CONTROLLERS     1
#endif

/* max. no. class drivers */
#define USBH_MAX_CLASS_DRIVERS        6

/* max. no. external hubs */
#define USBH_MAX_EXT_HUBS             2

/* max. no. ports */
#define USBH_MAX_PORTS                10

/* max. no. interfaces on a device */
#define USBH_MAX_INTERFACE_PER_DEVICE 16 //8 //Kevin for UVC

/* Check interval for transfer timeouts. 0 means timeout handling is disabled. */
#define USBH_TIMEOUT_CHECK_INTERVAL   10

/* Buffer size where to read config descriptor needs to be set to max. EP0 */
/* packet size + max. single descriptor size within the config descriptor. */
/* 128 should always be enough.                                            */
#define USBH_CFG_DESC_BUF_SIZE        256 // 128 // Irene Lin: modify for L610 4G module  

/* Set to 1 if Service Request Protocol and Host Negotiation Protocol supported */
#define USBH_OTG_SRPHNP_SUPPORT                   0

/* Set this to 1 to enable down rounding the polling interval to the greatest */
/* power of 2 for interrupt endpoints.                                        */
#define USBH_ENABLE_INT_POLLING_INTERVAL_ROUNDING 0

/* Set the size of the HC area in the USB host transfer structure */
#define USBH_HC_AREA_SIZE_IN_OCTECTS              16

#endif /* ifndef _CONFIG_USB_HOST_H */

