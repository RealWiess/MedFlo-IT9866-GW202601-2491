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
#ifndef _API_USBH_RNDIS_H
#define _API_USBH_RNDIS_H

#include <stdint.h>
#include "api_usb_host.h"
#include "api_nwdriver.h"

#include "../version/ver_usbh_rndis.h"
#if VER_USBH_RNDIS_MAJOR != 3 || VER_USBH_RNDIS_MINOR != 7
 #error Incompatible USBH_RNDIS version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif
#include "../version/ver_nwdriver.h"
#if VER_NWDRIVER_MAJOR != 5
 #error "Unsupported ETHDRIVER version."
#endif

#ifdef __cplusplus
extern "C" {
#endif

t_nwdriver_ret usbh_rndis_nwdrv_init ( uint32_t param, t_nwdriver * * const pp_nwdriver );

int usbh_rndis_register_ntf ( t_usbh_unit_id id, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_rndis_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_rndis_get_port_hdl ( t_usbh_unit_id uid );

int usbh_rndis_init ( void );
int usbh_rndis_start ( void );
int usbh_rndis_stop ( void );
int usbh_rndis_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_RNDIS_H */

