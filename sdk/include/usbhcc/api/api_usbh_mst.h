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
#ifndef _API_USBH_MST_H
#define _API_USBH_MST_H

#include "../psp/include/psp_types.h"
#include "api_usb_host.h"

#include "../version/ver_usbh_mst.h"
#if VER_USBH_MST_MAJOR != 3 || VER_USBH_MST_MINOR != 17
 #error Incompatible USBH_MST version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define USBH_SCSI_COM_NAME "USBH_MST"

int usbh_mst_register_ntf ( t_usbh_unit_id id, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_mst_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_mst_get_port_hdl ( t_usbh_unit_id uid );

int usbh_mst_init ( void );
int usbh_mst_start ( void );
int usbh_mst_stop ( void );
int usbh_mst_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_MST_H */

