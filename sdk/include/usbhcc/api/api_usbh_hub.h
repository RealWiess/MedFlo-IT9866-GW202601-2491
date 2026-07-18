/***************************************************************************
 *
 *            Copyright (c) 2010-2023 by Tuxera
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
#ifndef _API_USBH_HUB_H
#define _API_USBH_HUB_H

#include <stdint.h>
#include "api_usb_host.h"

#include "../version/ver_usbh_hub.h"
#if VER_USBH_HUB_MAJOR != 2 || VER_USBH_HUB_MINOR != 8
 #error Incompatible USBH_HUB version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

int usbh_hub_register_ntf ( t_usbh_unit_id id, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_hub_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_hub_get_port_hdl ( t_usbh_unit_id uid );

int usbh_hub_init ( void );
int usbh_hub_start ( void );
int usbh_hub_stop ( void );
int usbh_hub_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_HUB_H */

