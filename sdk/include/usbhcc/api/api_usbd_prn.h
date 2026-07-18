/***************************************************************************
 *
 *            Copyright (c) 2011-2021 by HCC Embedded
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


#ifndef _API_USBD_PRN_H_
#define _API_USBD_PRN_H_

#include <stdint.h>
#include <stddef.h>

#include "../version/ver_usbd_prn.h"
#if VER_USBD_PRN_MAJOR != 2 || VER_USBD_PRN_MINOR != 4
 #error "Invalid USBD_PRN version"
#endif


#define USBD_PRN_SUCCESS          0
#define USBD_PRN_ERROR            1
#define USBD_PRN_BUSY             2

#define USBD_PRN_PST_PAPER_EMPTY  ( 1u << 5 )
#define USBD_PRN_PST_SELECT       ( 1u << 4 )
#define USBD_PRN_PST_NOT_ERROR    ( 1u << 3 )

int usbd_prn_set_port_state ( uint8_t state_mask );
int usbd_prn_get_port_state ( uint8_t * state_mask );

int usbd_prn_receive ( void * buffer, uint32_t length, uint32_t * read );
uint32_t usbd_prn_get_rx_num ( void );
int usbd_prn_send ( void * buffer, uint32_t length );
int usbd_prn_send_state ( void );

void  usbd_prn_set_rst_cb( void ( * fn )( uint32_t param ), uint32_t param );
void  usbd_prn_set_start_stop_cb( void ( * fn )( uint32_t param ), uint32_t param );

int usbd_prn_is_active ( void );

int usbd_prn_init ( void );
int usbd_prn_start ( void );
int usbd_prn_stop ( void );
int usbd_prn_delete ( void );

#endif /* _API_USBD_PRN_H_ */
