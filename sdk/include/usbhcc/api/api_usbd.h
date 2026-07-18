/***************************************************************************
 *
 *            Copyright (c) 2008-2023 by Tuxera
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
#ifndef _API_USBD_H_
#define _API_USBD_H_

#include "../psp/include/psp_types.h"
#include "../config/config_usbd_config.h"
#include "../config/config_usbd.h"

#if USBD_ISOCHRONOUS_SUPPORT
 #include "api_hcc_rngbuf.h"
#endif
#if USBD_SOFTMR_SUPPORT
 #include "api_usbd_sof_timer.h"
#endif

#include "../version/ver_usbd.h"
#if ( VER_USBD_MAJOR != 3 ) || ( VER_USBD_MINOR != 32 )
 #error "Invalid USBD version."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* return codes */
#define USBD_SUCCESS      0
#define USBD_ERROR        1

/* USB driver state values.*/
#define USBDST_DISABLED   0        /* State after usbd_init() */
#define USBDST_DEFAULT    1        /* State after USB reset */
#define USBDST_ADDRESSED  2        /* State after set address */
#define USBDST_CONFIGURED 3        /* State after set config */
#define USBDST_SUSPENDED  4        /* State after suspend */

/* device states */
typedef enum
{
  usbdcst_invalid       /* invalid state, newer sent */
  , usbdcst_offline     /* cable not connected stack stopped */
  , usbdcst_ready       /* cable not connected stack started */
  , usbdcst_stopped     /* cable connected, usbd stack stopped */
  , usbdcst_not_cfg     /* not configured */
  , usbdcst_cfg         /* configured */
  , usbdcst_suspend     /* suspended */
  , usbdcst_suspend_hic /* suspended hi current draw enabled */
} usbd_conn_state_t;

/* Type of return value for most API functions. */
typedef int  usbd_error_t;

#if ( USBD_ISOCHRONOUS_SUPPORT != 0 )
typedef struct
{
  void    * data;
  uint32_t  size;
  uint32_t  nbytes;
  uint32_t  rd_ndx;
} usbd_iso_buffer_t;
#endif


/*
* Function name :
*   uint8_t * ( * t_usbd_get_user_string_cb )( uint16_t string_idx, uint16_t lang_id );
*
* Return :
*   Unicode USB string.
*   1st byte: length of string (bLength)
*   2nd byte: descriptor type (bDescriptorType, always 3),
*   other bytes: Unicode string
*
* Input :
*   string_idx : Index defined in USB descriptor (for example iManufacturer)
*   lang_id : Language ID
*
* Output :
*   none
*
* Description :
*   Generate Unicode USB string.
*/
typedef uint8_t * ( * t_usbd_get_user_string_cb )( uint16_t string_idx, uint16_t lang_id );

typedef struct
{
  uint16_t                   string_idx;
  t_usbd_get_user_string_cb  get_user_string_cb;
} t_usbd_user_string;

/* Table of callbacks to get USB strings */
extern const t_usbd_user_string  usbd_user_string_table[];

/****************** Callback function prototypes ******************/


/* Pull-up control call-back.
  Called by the driver to enable or disable the pull-up resistor. Some
  implementations hawe on-chip pull-up resistor. In this case this function
  will never be called. */
void usbd_pup_on_off ( int on );


/* Power status call-back.
   Called by the driver to query if the device is currently running self
   powered or not (is powered from the USB). */
int usbd_is_self_powered ( void );


/* This is a call-back function that tells current device state */
void usbd_conn_state ( usbd_conn_state_t new_state );

#if 1 // Irene Lin
int usbd_is_dbg(void);
#endif

/******************** API function prototypes **********************/


/* Initialize USB device stack.
    This function will allocate all dynamic resources (events, mutexes, etc)
    and preconfigure the hardware for normal operation. This functiol calls
    the _init() function of all USB modules. */
int usbd_init ( void );


/* Start USB device stack.

    This function enables normal driver operation.
       - pull-up resistor is enabled to make the host detect the device
       - processing of standard requests is enabled
       - enable USB related interrupt generation */
int usbd_start ( const usbd_config_t * conf );


/* Stop USB device stack.

    Stop normal operation.
       -disable pull-up resistor (host detects device removal)
       -processing of standard requests is stopped
       -USB related interrupt generation is disabled
       -USB hardware is out to low-power mode if possible */
int usbd_stop ( void );


/* Kill USB device stack.

    Free allocated resources. Call to any other driver function than usbd_init()
    is illegal after this function is called. */
int usbd_delete ( void );


/* Query current driver state.

    This function returns USBDST_XXX values. Please see the documentation of
    these for more details. */
int usbd_get_state ( void );


/* Tell current VBUS state to USB stack. When VBUS is detected this function
   shall be called with nonzero paramether value. When VBUS removed the function
   shall be called with paramether value 0. */
void usbd_vbus ( int on, int in_irq_context );


#if USBD_REMOTE_WAKEUP

/* Do a remote wakeup if possible. */
int usbd_remote_wakeup ( void );
#endif


#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_USBD_H_ */

