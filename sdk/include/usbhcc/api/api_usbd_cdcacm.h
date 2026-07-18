/***************************************************************************
 *
 *            Copyright (c) 2006-2020 by HCC Embedded
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
#ifndef _API_USBD_CDCACM_H_
#define _API_USBD_CDCACM_H_

#include "../config/config_usbd_cdcacm.h"
#include "../version/ver_usbd_cdcacm.h"

#if VER_USBD_CDCACM_MAJOR != 5 || VER_USBD_CDCACM_MINOR != 6
 #error "Invalid USBD_CDCACM version."
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parity values for line_coding_t */
#define LCT_PARITY_NONE             0
#define LCT_PARITY_ODD              1
#define LCT_PARITY_EVEN             2
#define LCT_PARITY_MARK             3
#define LCT_PARITY_SPACE            4

/* Stop bit configurations for line_coding_t */
#define LCT_STOP_1                  0
#define LCT_STOP_15                 1
#define LCT_STOP_2                  2

/* Return codes */
#define USBD_CDCACM_SUCCESS         0
#define USBD_CDCACM_ERROR           1
#define USBD_CDCACM_ERR_NOT_PRESENT 2
#define USBD_CDCACM_BUSY            3


/* Data carrier detect. Asserted when a connection has been established with
   remote equipment. */
#define LS_DCD                      ( 1 << 0 )

/* Data Set Ready. Asserted to indicate an active connection. */
#define LS_DSR                      ( 1 << 1 )

/* Break detection state. */
#define LS_BREAK                    ( 1 << 2 )

/* Ring indicator state. */
#define LS_RING                     ( 1 << 3 )

/* Framing error detected. */
#define LS_FE                       ( 1 << 4 )

/* Parity error detected. */
#define LS_PE                       ( 1 << 5 )

/* Overrun error detected. */
#define LS_OE                       ( 1 << 6 )

/* Control line state flag: Data Terminal Ready */
#define CLS_DTR                     ( 1 << 0 )

/* Control line state flag: Request to Send */
#define CLS_RTS                     ( 1 << 1 )

/* CDC ACM return type */
typedef int  t_usbd_cdcacm_ret;

/* Line coding structure for cdcacm_get_line_coding(), cdcacm_set_line_coding() */
typedef struct
{
  uint32_t  cdcacm_line_bps;        /* Baud rate */
  uint8_t   cdcacm_line_n_data;     /* Number of data bits */
  uint8_t   cdcacm_line_n_stp;      /* Number of stop bits */
  uint8_t   cdcacm_line_parity;     /* Parity */
} t_usbd_cdcacm_line_coding;

typedef enum
{
  USBD_CDCACM_NTF_CONNECT                   /* Line connect */
  , USBD_CDCACM_NTF_DISCONNECT              /* Line disconnect */
  , USBD_CDCACM_NTF_SET_LINE_CODING         /* Set Line Coding received */
  , USBD_CDCACM_NTF_BREAK                   /* Break */
  , USBD_CDCACM_NTF_RX                      /* Rx finifshed */
  , USBD_CDCACM_NTF_TX                      /* Tx finished */
  , USBD_CDCACM_NTF_SET_CONTROL_LINE_STATE  /* Set Control Line State received */
} t_usbd_cdcacm_ntf_type;

typedef enum
{
  USBD_CDCACM_RXMODE_NORMAL       /* Receiving into the internal CDC buffer */
  , USBD_CDCACM_RXMODE_DIRECT     /* Receiving directly into the user buffer */
} t_usbd_cdcacm_rx_mode;

typedef void ( * t_usbd_cdcacm_ntf_fn )( const uint8_t line, const t_usbd_cdcacm_ntf_type ntf_type );

/* Initialize class driver. */
t_usbd_cdcacm_ret usbd_cdcacm_init ( void );

/* Start class driver. */
t_usbd_cdcacm_ret usbd_cdcacm_start ( void );

/* Stop class driver. */
t_usbd_cdcacm_ret usbd_cdcacm_stop ( void );

/* Delete class driver. */
t_usbd_cdcacm_ret usbd_cdcacm_delete ( void );

/* Returns one if CDC line is active. */
t_usbd_cdcacm_ret usbd_cdcacm_present ( const uint8_t line );

/* Send multiple characters. */
t_usbd_cdcacm_ret usbd_cdcacm_send ( const uint8_t line, uint8_t * const p_buf, const uint32_t len );

/* Return status of previous transfer. */
t_usbd_cdcacm_ret usbd_cdcacm_send_status ( const uint8_t line );

/* Start a direct reception. */
t_usbd_cdcacm_ret usbd_cdcacm_receive_start ( const uint8_t line, uint8_t * const p_buf, const uint32_t len );

/* Read multiple characters. */
t_usbd_cdcacm_ret usbd_cdcacm_receive ( const uint8_t line, uint8_t * const p_buf, const uint32_t len_req, uint32_t * const p_len_done );

/* Get status of the current reception */
t_usbd_cdcacm_ret usbd_cdcacm_receive_status ( const uint8_t line, uint32_t * const p_len );

/* Set Rx mode to USBD_CDCACM_RXMODE_NORMAL or USBD_CDCACM_RXMODE_DIRECT */
t_usbd_cdcacm_ret usbd_cdcacm_set_rx_mode ( const uint8_t line, const t_usbd_cdcacm_rx_mode mode );

/* Register notification callback functions. */
t_usbd_cdcacm_ret usbd_cdcacm_reg_ntf_fn ( const t_usbd_cdcacm_ntf_type ntf, const t_usbd_cdcacm_ntf_fn ntf_fn );

/* Get current active line coding. */
t_usbd_cdcacm_ret usbd_cdcacm_get_line_coding ( const uint8_t line, t_usbd_cdcacm_line_coding * const p_ln_coding );

/* Set line coding. */
t_usbd_cdcacm_ret usbd_cdcacm_set_line_coding ( const uint8_t line, t_usbd_cdcacm_line_coding * const p_ln_coding );

/* control line state flags (DTR, RTS) set by the host */
t_usbd_cdcacm_ret usbd_cdcacm_get_control_line_state ( const uint8_t line, uint16_t * const p_flags );

/* Set line state flags. See LS_XXXX. */
t_usbd_cdcacm_ret usbd_cdcacm_set_lsflags ( const uint8_t line, const uint8_t flags );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBD_CDCACM_H_ */
