/***************************************************************************
 *
 *            Copyright (c) 2010-2019 by HCC Embedded
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
#ifndef _API_USBH_CDCACM_H
#define _API_USBH_CDCACM_H

#include <stdint.h>
#include "api_usb_host.h"

#include "../version/ver_usbh_cdcacm.h"
#if VER_USBH_CDCACM_MAJOR != 2 || VER_USBH_CDCACM_MINOR != 13
 #error Incompatible USBH_CDCACM version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define USBH_CDCSER_BITS_5       5
#define USBH_CDCSER_BITS_6       6
#define USBH_CDCSER_BITS_7       7
#define USBH_CDCSER_BITS_8       8
#define USBH_CDCSER_BITS_16      16

#define USBH_CDCSER_PARITY_NONE  0
#define USBH_CDCSER_PARITY_ODD   1
#define USBH_CDCSER_PARITY_EVEN  2
#define USBH_CDCSER_PARITY_MARK  3
#define USBH_CDCSER_PARITY_SPACE 4

#define USBH_CDCSER_STOP_1       0
#define USBH_CDCSER_STOP_1_5     1
#define USBH_CDCSER_STOP_2       2

#define USBH_CDCSER_LST_OE       ( 1u << 6 )
#define USBH_CDCSER_LST_PE       ( 1u << 5 )
#define USBH_CDCSER_LST_FE       ( 1u << 4 )
#define USBH_CDCSER_LST_RINGS    ( 1u << 3 )
#define USBH_CDCSER_LST_BREAK    ( 1u << 2 )
#define USBH_CDCSER_LST_DSR      ( 1u << 1 )
#define USBH_CDCSER_LST_DCD      ( 1u << 0 )

/***** notifications *****/
#define USBH_NTF_CDCACM_RX       ( USBH_NTF_CD_BASE + 0 )
#define USBH_NTF_CDCACM_TX       ( USBH_NTF_CD_BASE + 1 )
#define USBH_NTF_CDCACM_NTF      ( USBH_NTF_CD_BASE + 2 )


int usbh_cdcacm_send ( t_usbh_unit_id uid, uint8_t * buf, uint32_t length );
int usbh_cdcacm_receive ( t_usbh_unit_id uid, uint8_t * buf, uint32_t max_length, uint32_t * rlength );
int usbh_cdcacm_get_send_state ( t_usbh_unit_id uid );
int usbh_cdcacm_get_receive_state ( t_usbh_unit_id uid );
uint32_t usbh_cdcacm_rx_chars ( t_usbh_unit_id uid );

int usbh_cdcacm_get_line_coding ( t_usbh_unit_id uid, uint32_t * br, uint8_t * b, uint8_t * p, uint8_t * s );
int usbh_cdcacm_set_line_coding ( t_usbh_unit_id uid, uint32_t br, uint8_t b, uint8_t p, uint8_t s );
int usbh_cdcacm_set_control_line_state ( t_usbh_unit_id uid, uint8_t rts, uint8_t dtr );

uint16_t usbh_cdcacm_get_serial_state ( t_usbh_unit_id uid );

int usbh_cdcacm_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_cdcacm_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_cdcacm_get_port_hdl ( t_usbh_unit_id uid );

int usbh_cdcacm_init ( void );
int usbh_cdcacm_start ( void );
int usbh_cdcacm_stop ( void );
int usbh_cdcacm_delete ( void );


#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_USBH_CDCACM_H */


