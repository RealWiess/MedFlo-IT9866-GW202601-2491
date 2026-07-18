/***************************************************************************
 *
 *            Copyright (c) 2010-2020 by HCC Embedded
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
#ifndef _API_USBH_CDCECM_H
#define _API_USBH_CDCECM_H

#include <stdint.h>
#include "api_usb_host.h"
#include "../config/config_usbh_cdcecm.h"

#include "../version/ver_usbh_cdcecm.h"
#if VER_USBH_CDCECM_MAJOR != 2 || VER_USBH_CDCECM_MINOR != 10
 #error Incompatible USBH_CDCECM version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

/********** Ethernet statistics **********/
#define USBH_CDCECM_XMIT_OK                   1
#define USBH_CDCECM_RCV_OK                    2
#define USBH_CDCECM_XMIT_ERROR                3
#define USBH_CDCECM_RCV_ERROR                 4
#define USBH_CDCECM_RCV_NO_BUFFER             5
#define USBH_CDCECM_DIRECTED_BYTES_XMIT       6
#define USBH_CDCECM_DIRECTED_FRAMES_XMIT      7
#define USBH_CDCECM_MULTICAST_BYTES_XMIT      8
#define USBH_CDCECM_MULTICAST_FRAMES_XMIT     9
#define USBH_CDCECM_BROADCAST_BYTES_XMIT      10
#define USBH_CDCECM_BROADCAST_FRAMES_XMIT     11
#define USBH_CDCECM_DIRECTED_BYTES_RCV        12
#define USBH_CDCECM_DIRECTED_FRAMES_RCV       13
#define USBH_CDCECM_MULTICAST_BYTES_RCV       14
#define USBH_CDCECM_MULTICAST_FRAMES_RCV      15
#define USBH_CDCECM_BROADCAST_BYTES_RCV       16
#define USBH_CDCECM_BROADCAST_FRAMES_RCV      17
#define USBH_CDCECM_RCV_CRC_ERROR             18
#define USBH_CDCECM_TRANSMIT_QUEUE_LENGTH     19
#define USBH_CDCECM_RCV_ERROR_ALIGNMENT       20
#define USBH_CDCECM_XMIT_ONE_COLLISION        21
#define USBH_CDCECM_XMIT_MORE_COLLISIONS      22
#define USBH_CDCECM_XMIT_DEFERRED             23
#define USBH_CDCECM_XMIT_MAX_COLLISIONS       24
#define USBH_CDCECM_RCV_OVERRUN               25
#define USBH_CDCECM_XMIT_UNDERRUN             26
#define USBH_CDCECM_XMIT_HEARTBEAT_FAILURE    27
#define USBH_CDCECM_XMIT_TIMES_CRS_LOST       28
#define USBH_CDCECM_XMIT_LATE_COLLISIONS      29

/********** Ethernet packet filters **********/
#define USBH_CDCECM_PACKET_TYPE_PROMISCUOUS   ( 1u << 0 )
#define USBH_CDCECM_PACKET_TYPE_ALL_MULTICAST ( 1u << 1 )
#define USBH_CDCECM_PACKET_TYPE_DIRECTED      ( 1u << 2 )
#define USBH_CDCECM_PACKET_TYPE_BROADCAST     ( 1u << 3 )
#define USBH_CDCECM_PACKET_TYPE_MULTICAST     ( 1u << 4 )

/***** notifications *****/
#define USBH_NTF_CDCECM_RX                    ( USBH_NTF_CD_BASE + 0 )
#define USBH_NTF_CDCECM_TX                    ( USBH_NTF_CD_BASE + 1 )
#define USBH_NTF_CDCECM_NTF                   ( USBH_NTF_CD_BASE + 2 )


int usbh_cdcecm_get_max_segment_size ( t_usbh_unit_id uid, uint16_t * max_segment_size );
int usbh_cdcecm_get_mac_address ( t_usbh_unit_id uid, uint8_t * mac_address );
int usbh_cdcecm_get_network_state ( t_usbh_unit_id uid, uint8_t * state );
int usbh_cdcecm_get_connection_speed ( t_usbh_unit_id uid, uint32_t * usbitrate, uint32_t * dsbitrate );
int usbh_cdcecm_set_eth_mcast_filters ( t_usbh_unit_id uid, uint16_t n, uint8_t * filters );
int usbh_cdcecm_set_eth_packet_filter ( t_usbh_unit_id uid, uint16_t flags );
int usbh_cdcecm_get_eth_statistics ( t_usbh_unit_id uid, uint16_t stat, uint32_t * val );

int usbh_cdcecm_send ( t_usbh_unit_id uid, uint8_t * buf, uint32_t length );
#if ( USBH_CDCECM_EXTERNAL_BUF == 0u )
int usbh_cdcecm_receive ( t_usbh_unit_id uid, uint8_t * buf, uint32_t max_length, uint32_t * rlength );
#else
int usbh_cdcecm_receive ( t_usbh_unit_id uid, uint8_t * * pp_buf, uint32_t * rlength );
int usbh_cdcecm_add_buf ( t_usbh_unit_id uid, uint8_t buf[], uint32_t length, uint8_t b_start );
#endif
int usbh_cdcecm_get_send_state ( t_usbh_unit_id uid );
int usbh_cdcecm_get_receive_state ( t_usbh_unit_id uid );
uint32_t usbh_cdcecm_rx_chars ( t_usbh_unit_id uid );

int usbh_cdcecm_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_cdcecm_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_cdcecm_get_port_hdl ( t_usbh_unit_id uid );

int usbh_cdcecm_init ( void );
int usbh_cdcecm_start ( void );
int usbh_cdcecm_stop ( void );
int usbh_cdcecm_delete ( void );

#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_USBH_CDCECM_H */

