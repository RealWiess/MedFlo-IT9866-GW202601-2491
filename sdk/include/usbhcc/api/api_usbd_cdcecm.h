/****************************************************************************
 *
 *            Copyright (c) 2024 by ITE
 *
 * This software is developed using the HCC USB Device Stack as a lower-level
 * driver framework. The ECM Class implementation is an independent work
 * developed by ITE, based on the USB CDC ECM 1.2 specification.
 *
 * The underlying USB device stack is copyrighted by HCC Embedded and is used
 * under license.
 *
 ***************************************************************************/
/****************************************************************************
 *
 *            Copyright (c) 2021 by HCC Embedded
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
#ifndef API_USBD_CDCECM_H
#define API_USBD_CDCECM_H

#include "../psp/include/psp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
*  Defines section
*************************************************************************/
/* Return codes */
#define USBD_CDCECM_SUCCESS       0
#define USBD_CDCECM_ERROR         1
#define USBD_CDCECM_BUSY          2

/* CDC-ECM related events */
#define USBD_CDCECM_RX_FLAG       1u
#define USBD_CDCECM_TX_FLAG       2u
#define USBD_CDCECM_STATECHG_FLAG 4u

/************************************************************************
* Typedefs
*************************************************************************/
/* State change callback function type */
typedef enum
{
  USBD_CDCECM_NTF_CONNECT
  , USBD_CDCECM_NTF_DISCONNECT
  , USBD_CDCECM_NTF_RX
  , USBD_CDCECM_NTF_TX
} t_usbd_cdcecm_ntf;

typedef int ( * t_usbd_cdcecm_cb)( t_usbd_cdcecm_ntf ntf );

typedef int  t_usbd_cdcecm_ret;


/************************************************************************
* Function Prototype Section
*************************************************************************/

t_usbd_cdcecm_ret usbd_cdcecm_init ( void );
t_usbd_cdcecm_ret usbd_cdcecm_start ( void );
t_usbd_cdcecm_ret usbd_cdcecm_stop ( void );
t_usbd_cdcecm_ret usbd_cdcecm_delete ( void );

t_usbd_cdcecm_ret usbd_cdcecm_register_ntf ( t_usbd_cdcecm_ntf ntf, t_usbd_cdcecm_cb ntf_fn );
t_usbd_cdcecm_ret usbd_cdcecm_send_frame ( uint8_t * const p_buf, const uint16_t len );
t_usbd_cdcecm_ret usbd_cdcecm_receive_frame ( uint8_t ** p_buf_addr, uint32_t * p_rlength );
t_usbd_cdcecm_ret usbd_cdcecm_receive_frame_done ( void );
t_usbd_cdcecm_ret usbd_cdcecm_is_network_connection_up ( void );

#endif /* ifndef API_USBD_CDCECM_H */
