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
#ifndef _API_USBD_RAW_H_
#define _API_USBD_RAW_H_

#include <stdint.h>

#include "../config/config_usbd_raw.h"

#include "../version/ver_usbd_raw.h"
#if VER_USBD_RAW_MAJOR != 4 || VER_USBD_RAW_MINOR != 1
 #error "Invalid USBD_RAW version."
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define USBD_RAW_SUCCESS           0
#define USBD_RAW_BUSY              1
#define USBD_RAW_NOT_PRESENT       2
#define USBD_RAW_NOT_STARTED       3
#define USBD_RAW_ERROR             4

/* Bitmask for the Request type field of the setup packet (bmRequestType) */
#define USBD_RAW_USBRQT_DIR_IN     ( 1u << 7 ) /* IN request */
#define USBD_RAW_USBRQT_DIR_OUT    ( 0u << 7 ) /* OUT request */
#define USBD_RAW_USBRQT_DIR_MASK   ( 1u << 7 ) /* Mask for request */
#define USBD_RAW_USBRQT_TYP_STD    ( 0u << 5 ) /* Standard request */
#define USBD_RAW_USBRQT_TYP_CLASS  ( 1u << 5 ) /* Class-specific request */
#define USBD_RAW_USBRQT_TYP_VENDOR ( 2u << 5 ) /* Vendor-specific request */
#define USBD_RAW_USBRQT_TYP_MASK   ( 3u << 5 ) /* Mask for request type bits */
#define USBD_RAW_USBRQT_RCP_DEVICE ( 0u << 0 ) /* Recipient is the device */
#define USBD_RAW_USBRQT_RCP_IFC    ( 1u << 0 ) /* Recipient is an interface */
#define USBD_RAW_USBRQT_RCP_EP     ( 2u << 0 ) /* Recipient is an endpoint */
#define USBD_RAW_USBRQT_RCP_OTHER  ( 3u << 0 ) /* Recipient is something else */
#define USBD_RAW_USBRQT_RCP_MASK   ( 3u << 0 ) /* Mask for recipient bits */

typedef enum
{
  /* Passed to the callback which was registered using */
  /* usbd_raw_register_ep0_callback() */
  USBD_RAW_EP0_EVENT_SETUP_RECEIVED = 1

                                      /* Passed to transfer_finished_callback function when transfer finished */
                                      /* successfully */
  , USBD_RAW_EP0_EVENT_TRANSFER_FINISHED_SUCCESS

  /* Passed to transfer_finished_callback function when error occurred */
  /* during transfer */
  , USBD_RAW_EP0_EVENT_TRANSFER_FINISHED_ERROR
} t_usbd_raw_ep0_event;

/* Answers returned by callback function */
typedef enum
{
  USBD_RAW_SETUP_ANSWER_STALL = 1  /* request not recognized/respond stall */
  , USBD_RAW_SETUP_ANSWER_ACK      /* acknowledge (no data transfer) */
  , USBD_RAW_SETUP_ANSWER_DATA_IN  /* write from device's perspective (data transfer from device to host) */
  , USBD_RAW_SETUP_ANSWER_DATA_OUT /* read from device's perspective (data transfer from host to device) */
} t_usbd_raw_setup_answer;

typedef struct t_usbd_raw_setup_data_tag  t_usbd_raw_setup_data;
typedef void                          ( * t_usbd_raw_ep0_cb ) ( t_usbd_raw_setup_data * p_setup_data );


/* Structure to hold setup transaction data.
   Used in ep0 callback functions. */
struct t_usbd_raw_setup_data_tag
{
  /* Inputs to the callback function */
  t_usbd_raw_ep0_event  ep0_event;

  /* The SETUP packet's field */
  uint8_t   bmRequestType;
  uint8_t   bRequest;
  uint16_t  wValue;
  uint16_t  wIndex;
  uint16_t  wLength;

  /* Outputs from the callback function */
  t_usbd_raw_setup_answer  answer;
  uint8_t                * p_buffer;      /* Pointer to buffer, used when USBD_RAW_SETUP_ANSWER_IN/OUT answered */
  uint32_t                 buffer_length; /* Size of buffer in bytes */
  uint32_t                 user_data;     /* Optional, can be used by transfer_finished_callback function */
  /* Optional: function to be called when transfer finished */
  /* Only applicable when USBD_RAW_SETUP_ANSWER_DATA_IN or */
  /* USBD_RAW_SETUP_ANSWER_DATA_OUT is used (there is data transfer) */
  t_usbd_raw_ep0_cb  transfer_finished_callback;
};

int usbd_raw_init ( void );
int usbd_raw_start ( void );
int usbd_raw_stop ( void );
int usbd_raw_delete ( void );

int usbd_raw_present ( uint8_t ch );

int usbd_raw_read ( uint8_t ch, uint8_t * p_buf, uint32_t blen );
int usbd_raw_read_state ( uint8_t ch, uint8_t b_block, uint32_t * p_rlen );

int usbd_raw_write ( uint8_t ch, uint8_t * p_buf, uint32_t blen, uint8_t b_skip_zlp );
int usbd_raw_write_state ( uint8_t ch, uint8_t b_block );

#if ( USBD_RAW_USE_INT_IN != 0 )
int usbd_raw_write_int ( uint8_t ch, uint8_t * p_buf, uint32_t blen, uint8_t b_skip_zlp );
int usbd_raw_write_int_state ( uint8_t ch, uint8_t b_block );
#endif

#if ( USBD_RAW_USE_INT_OUT != 0 )
int usbd_raw_read_int ( uint8_t ch, uint8_t * p_buf, uint32_t blen );
int usbd_raw_read_int_state ( uint8_t ch, uint8_t b_block, uint32_t * p_rlen );
#endif

void usbd_raw_register_ep0_callback ( t_usbd_raw_ep0_cb p_callback );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBD_RAW_H_ */

