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
#ifndef API_USBD_CDCNCM_H
#define API_USBD_CDCNCM_H


/************************************************************************
*  Include section
*************************************************************************/

#include "../psp/include/psp_types.h"
#include "../config/config_usbd_cdcncm.h"

#include "../version/ver_psp_types.h"
#if ( VER_PSP_TYPES_MAJOR != 1 )
 #error Incompatible PSP_TYPES version number!
#endif

#include "../version/ver_usbd_cdcncm.h"
#if VER_USBD_CDCNCM_MAJOR != 1 || VER_USBD_CDCNCM_MINOR != 2
 #error Incompatible USBD_CDCNCM verion number
#endif

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************
*  Defines section
*************************************************************************/

/* Module return codes */
#define USBD_CDCNCM_SUCCESS       0
#define USBD_CDCNCM_ERROR         1
#define USBD_CDCNCM_BUSY          2

/* CDC-NCM related events */
#define USBD_CDCNCM_RX_FLAG       1u
#define USBD_CDCNCM_TX_FLAG       2u
#define USBD_CDCNCM_STATECHG_FLAG 4u


/************************************************************************
* Typedefs
*************************************************************************/

/* State change callback function type */
typedef int ( * t_usbd_cdcncm_cb)( uint8_t ntf );

typedef enum
{
  USBD_CDCNCM_NTF_CONNECT
  , USBD_CDCNCM_NTF_DISCONNECT
  , USBD_CDCNCM_NTF_RX
  , USBD_CDCNCM_NTF_TX
} t_usbd_cdcncm_ntf;

typedef int  t_usbd_cdcncm_ret;


/************************************************************************
* Function Prototype Section
*************************************************************************/

t_usbd_cdcncm_ret usbd_cdcncm_init ( void );
t_usbd_cdcncm_ret usbd_cdcncm_start ( void );
t_usbd_cdcncm_ret usbd_cdcncm_stop ( void );
t_usbd_cdcncm_ret usbd_cdcncm_delete ( void );

t_usbd_cdcncm_ret usbd_cdcncm_send_frame ( uint8_t * const p_buf, const uint16_t len );
t_usbd_cdcncm_ret usbd_cdcncm_receive_frame ( uint8_t * p_buf, uint32_t buf_size, uint32_t * p_rlength );
void              usbd_cdcncm_set_link_status ( uint8_t link_state );
t_usbd_cdcncm_ret usbd_cdcncm_register_ntf ( t_usbd_cdcncm_ntf ntf, t_usbd_cdcncm_cb ntf_fn );
t_usbd_cdcncm_ret usbd_cdcncm_is_network_connection_up ( void );
t_usbd_cdcncm_ret usbd_cdcncm_get_connection_speed ( uint32_t * const p_link_speed );

#ifdef __cplusplus
}
#endif

#endif /* ifndef API_USBD_CDCNCM_H */
