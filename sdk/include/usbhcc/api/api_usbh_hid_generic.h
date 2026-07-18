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
#ifndef _API_USBH_HID_GENERIC_H
#define _API_USBH_HID_GENERIC_H

#include "../psp/include/psp_types.h"
#include "api_usb_host.h"
#include "api_usbh_hid.h"
#include "../config/config_usbh_hid.h"
#include "../config/config_usbh_hid_generic.h"

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




#if ( HID_GENERIC_ENABLE == 1 )




 #define USBH_NTF_HID_GENERIC_RX ( USBH_NTF_CD_BASE + 0 )




int usbh_hid_generic_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_hid_generic_present ( t_usbh_unit_id uid );

int usbh_hid_generic_get_uid ( t_usbh_port_hdl port_hdl, t_usbh_unit_id * p_uid );
t_usbh_hid_hdl usbh_hid_generic_get_hid_hdl ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_hid_generic_get_port_hdl ( t_usbh_unit_id uid );

typedef void ( * t_usbh_hid_generic_accept_report_ntf )( t_usbh_port_hdl port_hdl, uint8_t ifc_ndx, t_usbh_unit_id uid, t_report * report, uint8_t * accept );
void usbh_hid_generic_register_accept_report_ntf ( t_usbh_hid_generic_accept_report_ntf ntf );

typedef void ( * t_usbh_hid_generic_receive_ntf )( t_usbh_unit_id uid, uint8_t report_id, uint8_t * report_data, uint32_t report_size );
void usbh_hid_generic_register_receive_ntf ( t_usbh_hid_generic_receive_ntf ntf );

int usbh_hid_generic_get_report_count ( t_usbh_unit_id uid, uint8_t * report_count );

 #if RAW_REPORTS == 0
int usbh_hid_generic_get_report ( t_usbh_unit_id uid, uint8_t index, t_report * * report );
 #endif
int usbh_hid_generic_read_report ( t_usbh_unit_id uid, uint8_t report_id, uint8_t * buffer, uint8_t length );
int usbh_hid_generic_find_report ( t_usbh_unit_id uid, t_report_type report_type, uint8_t report_id, t_report * * report );
int usbh_hid_generic_write_report ( t_usbh_unit_id uid, uint8_t report_id, uint8_t * buffer, uint8_t length );
int usbh_hid_generic_it_ep_write_report ( t_usbh_unit_id uid, uint8_t * buffer, uint8_t length );




 #if USBH_HID_GENERIC_BUFFER_SUPPORT != 0


typedef void ( * t_hid_generic_cb )( t_usbh_port_hdl port_hdl, uint32_t param );

int usbh_hid_generic_register_cb ( t_hid_generic_cb cb, uint32_t param );
  #if RAW_REPORTS == 0
int usbh_hid_generic_get_value ( t_usbh_unit_id uid, uint16_t usage_page, uint8_t usage, int32_t * value );
  #endif
int usbh_hid_generic_read_in_report ( t_usbh_unit_id uid, uint8_t report_id, uint8_t * buffer, uint8_t length, uint8_t ctrl );


 #endif




#endif /* if ( HID_GENERIC_ENABLE == 1 ) */




#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_HID_GENERIC_H */

