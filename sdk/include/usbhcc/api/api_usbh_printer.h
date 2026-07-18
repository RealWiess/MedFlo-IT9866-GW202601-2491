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
#ifndef _USBH_PRINTER_H_
#define _USBH_PRINTER_H_

#include <stdint.h>
#include "api_usb_host.h"

#include "../version/ver_usbh_printer.h"
#if VER_USBH_PRINTER_MAJOR != 3 || VER_USBH_PRINTER_MINOR != 1
 #error Incompatible USBH_PRINTER version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* printer status bits */
#define PRN_PAPER_EMPTY       ( 1u << 5 )
#define PRN_SELECT            ( 1u << 4 )
#define PRN_NOT_ERROR         ( 1u << 3 )

#define USBH_PRN_WAIT_FOREVER 0         /* Timeout: wait forever */


int usbh_prn_send ( t_usbh_unit_id uid, uint8_t * buffer, uint32_t length, uint32_t * slength, uint32_t timeout_ms );
int usbh_prn_receive ( t_usbh_unit_id uid, uint8_t * buffer, uint32_t length, uint32_t * rlength, uint32_t timeout_ms );

int usbh_prn_read_id ( t_usbh_unit_id uid, uint8_t * buffer, uint32_t length, uint32_t * rlength );
int usbh_prn_soft_reset ( t_usbh_unit_id uid );
int usbh_prn_get_port_status ( t_usbh_unit_id uid, uint8_t * status );

int usbh_prn_print ( t_usbh_unit_id uid, uint8_t * buffer, uint32_t length, uint32_t timeout_ms );

int usbh_prn_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_prn_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_prn_get_port_hdl ( t_usbh_unit_id uid );

int usbh_prn_init ( void );
int usbh_prn_start ( void );
int usbh_prn_stop ( void );
int usbh_prn_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _USBH_PRINTER_H_ */

