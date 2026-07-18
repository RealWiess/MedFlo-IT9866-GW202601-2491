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
#ifndef _API_USBH_HID_MOUSE_H
#define _API_USBH_HID_MOUSE_H

#include "../psp/include/psp_types.h"
#include "api_usb_host.h"
#include "api_usbh_hid.h"
#include "../config/config_usbh_hid.h"
#include "../config/config_usbh_hid_mouse.h"

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 10
 #error Incompatible USBH_HID version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif




#if ( HID_MOUSE_ENABLE == 1 )




/***** notification *****/
 #define USBH_NTF_HID_MOUSE_RX ( USBH_NTF_CD_BASE + 0 )


/***** Mouse report *****/
typedef struct
{
  int32_t   dx;
  int32_t   dy;
  int32_t   dwheel;
  uint16_t  buttons;
  uint32_t  dx_attribute;
  uint32_t  dy_attribute;
  uint32_t  dwheel_attribute;
  uint32_t  buttons_attributes[MAX_MOUSE_BUTTONS];
} t_mouse_report;


int usbh_hid_mouse_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_hid_mouse_present ( t_usbh_unit_id uid );

int usbh_hid_mouse_get_uid ( t_usbh_port_hdl port_hdl, t_usbh_unit_id * p_uid );
t_usbh_hid_hdl usbh_hid_mouse_get_hid_hdl ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_hid_mouse_get_port_hdl ( t_usbh_unit_id uid );

int usbh_hid_mouse_get_report ( t_usbh_unit_id uid, t_mouse_report * p_mouse_report );
int usbh_hid_mouse_get_report_item ( t_usbh_unit_id uid, uint8_t item_index, t_report_item * * report_item );




#endif /* if ( HID_MOUSE_ENABLE == 1 ) */




#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_HID_MOUSE_H */

