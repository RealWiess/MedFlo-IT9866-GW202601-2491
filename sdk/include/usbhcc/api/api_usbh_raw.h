/***************************************************************************
 *
 *            Copyright (c) 2012-2018 by HCC Embedded
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
#ifndef _API_USBH_RAW_H_
#define _API_USBH_RAW_H_

#include "../psp/include/psp_types.h"

#include "api_usb_host.h"

#include "../version/ver_usbh_raw.h"
#if VER_USBH_RAW_MAJOR != 4 || VER_USBH_RAW_MINOR != 2
 #error Incompatible USBH_RAW version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*************************** return codes ***************************/
#define USBH_RAW_SUCCESS      0u
#define USBH_RAW_BUSY         1u
#define USBH_RAW_NOT_PRESENT  2u
#define USBH_RAW_NOT_STARTED  3u
#define USBH_RAW_ERROR        4u

#define USBH_RAW_MAX_CHANNELS 4u

/*************************** notifications **************************/
#define USBH_NTF_RAW_RX( ch ) ( USBH_NTF_CD_BASE + ch )
#define USBH_NTF_RAW_TX( ch ) ( USBH_NTF_CD_BASE + USBH_RAW_MAX_CHANNELS + ch )
#define USBH_NTF_RAW_INT_RX( ch ) ( USBH_NTF_CD_BASE + 2 * USBH_RAW_MAX_CHANNELS + ch )
#define USBH_NTF_RAW_INT_TX( ch ) ( USBH_NTF_CD_BASE + 3 * USBH_RAW_MAX_CHANNELS + ch )

/* Bitmask for the Request type field of the setup packet (bmRequestType) */
#define USBH_RAW_USBRQT_DIR_IN     ( 1u << 7 ) /* IN request */
#define USBH_RAW_USBRQT_DIR_OUT    ( 0u << 7 ) /* OUT request */
#define USBH_RAW_USBRQT_DIR_MASK   ( 1u << 7 ) /* Mask for request */
#define USBH_RAW_USBRQT_TYP_STD    ( 0u << 5 ) /* Standard request */
#define USBH_RAW_USBRQT_TYP_CLASS  ( 1u << 5 ) /* Class-specific request */
#define USBH_RAW_USBRQT_TYP_VENDOR ( 2u << 5 ) /* Vendor-specific request */
#define USBH_RAW_USBRQT_TYP_MASK   ( 3u << 5 ) /* Mask for request type bits */
#define USBH_RAW_USBRQT_RCP_DEVICE ( 0u << 0 ) /* Recipient is the device */
#define USBH_RAW_USBRQT_RCP_IFC    ( 1u << 0 ) /* Recipient is an interface */
#define USBH_RAW_USBRQT_RCP_EP     ( 2u << 0 ) /* Recipient is an endpoint */
#define USBH_RAW_USBRQT_RCP_OTHER  ( 3u << 0 ) /* Recipient is something else */
#define USBH_RAW_USBRQT_RCP_MASK   ( 3u << 0 ) /* Mask for recipient bits */

typedef struct
{
  /* The SETUP packet's field */
  uint8_t   bmRequestType;
  uint8_t   bRequest;
  uint16_t  wValue;
  uint16_t  wIndex;
  uint16_t  wLength;

  uint8_t * p_buffer;        /* Pointer to buffer (used when wLength is not zero) */
  uint32_t  received_length; /* Actually received bytes (used when usbh_raw_read_ctrl() is called) */
} t_usbh_raw_setup_data;

/*********************** Function prototypes ***********************/
int usbh_raw_init ( void );
int usbh_raw_start ( void );
int usbh_raw_stop ( void );
int usbh_raw_delete ( void );

int usbh_raw_present ( t_usbh_unit_id uid );
int usbh_raw_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn p_ntf_fn );
t_usbh_port_hdl usbh_raw_get_port_hdl ( t_usbh_unit_id uid );

int usbh_raw_read ( t_usbh_unit_id uid, uint8_t channel, uint8_t * pc_dst, uint32_t blen, uint8_t skip_zlp );
int usbh_raw_read_state ( t_usbh_unit_id uid, uint8_t channel, uint8_t b_block, uint32_t * p_rlen );

int usbh_raw_write ( t_usbh_unit_id uid, uint8_t channel, uint8_t * pc_buf, uint32_t blen, uint8_t skip_zlp );
int usbh_raw_write_state ( t_usbh_unit_id uid, uint8_t channel, uint8_t b_block );

int usbh_raw_write_ctrl ( t_usbh_unit_id uid, t_usbh_raw_setup_data * p_setup_data );
int usbh_raw_read_ctrl ( t_usbh_unit_id uid, t_usbh_raw_setup_data * p_setup_data );

int usbh_raw_read_int ( t_usbh_unit_id uid, uint8_t channel, uint8_t * p_buf, uint32_t blen, uint8_t b_skip_zlp );
int usbh_raw_read_int_state ( t_usbh_unit_id uid, uint8_t channel, uint8_t b_block, uint32_t * p_rlen );

int usbh_raw_write_int ( t_usbh_unit_id uid, uint8_t channel, uint8_t * p_buf, uint32_t blen, uint8_t b_skip_zlp );
int usbh_raw_write_int_state ( t_usbh_unit_id uid, uint8_t channel, uint8_t b_block );

int usbh_raw_get_ep_number ( t_usbh_unit_id uid, uint8_t channel, uint8_t direction, uint8_t * ep );
int usbh_raw_read_ep ( t_usbh_unit_id uid, uint8_t ep, uint8_t * pc_dst, uint32_t blen, uint8_t skip_zlp );
int usbh_raw_read_state_ep ( t_usbh_unit_id uid, uint8_t ep, uint8_t b_block, uint32_t * p_rlen );
int usbh_raw_write_ep ( t_usbh_unit_id uid, uint8_t ep, uint8_t * pc_buf, uint32_t blen, uint8_t skip_zlp );
int usbh_raw_write_state_ep ( t_usbh_unit_id uid, uint8_t ep, uint8_t b_block );

#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_USBH_RAW_H_ */


