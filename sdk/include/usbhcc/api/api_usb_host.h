/***************************************************************************
 *
 *            Copyright (c) 2011-2024 by Tuxera
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
#ifndef _API_USB_HOST_H
#define _API_USB_HOST_H

#include "../psp/include/psp_types.h"
#include "../config/config_usb_host.h"

#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3 || VER_USB_HOST_MINOR != 28
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*************************** error codes ***************************/
#define USBH_SUCCESS            0       /* no error occured */
#define USBH_SHORT_PACKET       1       /* IN transfer completed with short packet */
#define USBH_PENDING            2       /* transfer still pending */
#define USBH_ERR_BUSY           3       /* another transfer in progress */
#define USBH_ERR_DIR            4       /* transfer direction error */
#define USBH_ERR_TIMEOUT        5       /* transfer timed out */
#define USBH_ERR_TRANSFER       6       /* transfer failed to complete */
#define USBH_ERR_TRANSFER_FULL  7       /* can't process more transfers */
#define USBH_ERR_SUSPENDED      8       /* host controller is suspended */
#define USBH_ERR_HC_HALTED      9       /* host controller is halted */
#define USBH_ERR_REMOVED        10      /* transfer finished due to device removal */
#define USBH_ERR_PERIODIC_LIST  11      /* periodic list error */
#define USBH_ERR_RESET_REQUEST  12      /* reset request during enumeration */
#define USBH_ERR_RESOURCE       13      /* OS resource error */
#define USBH_ERR_INVALID        14      /* invalid identifier/type (HC, EP HDL, etc.) */
#define USBH_ERR_NOT_AVAILABLE  15      /* not available */
#define USBH_ERR_INVALID_SIZE   16      /* invalid size */
#define USBH_ERR_NOT_ALLOWED    17      /* operation not allowed */
#define USBH_ERROR              18      /* general error */

/************************ connection states ************************/
#define USBH_STATE_FREE         0x00u
#define USBH_STATE_INVALID      USBH_STATE_FREE
#define USBH_STATE_DISCONNECTED 0x01u
#define USBH_STATE_CONNECTED    0x02u
#define USBH_STATE_SUSPENDED    0x04u
#define USBH_STATE_ISUSPENDED   0x08u
#define USBH_STATE_RWKUP        0x10u
#define USBH_STATE_OVERCURRENT  0x20u
#define USBH_STATE_ENUM         0x40u
#define USBH_STATE_CHANGED      0x80u
#define USBH_STATE_OPERATIONAL  0x0100u

/************************ connection speeds ************************/
#define USBH_LOW_SPEED          1
#define USBH_FULL_SPEED         2
#define USBH_HIGH_SPEED         3

/****************** string descriptor definitions ******************/
/* string descriptors */
#define USBH_STR_MANUFACTURER   ( (uint16_t)0xFFFFu )
#define USBH_STR_PRODUCT        ( (uint16_t)0xFFFEu )
#define USBH_STR_SERIAL         ( (uint16_t)0xFFFDu )

/* language ID */
#define USBH_LANG_ID_DEFAULT    0

/***************************** unit ID *****************************/
typedef uint8_t  t_usbh_unit_id;

/********************* port handle/information *********************/
#define USBH_PORT_HDL_INVALID NULL

typedef void * t_usbh_port_hdl;

typedef struct
{
  uint16_t  state;                       /* USBH_STATE_... */
  uint8_t   hc_uid;                      /* host controller ID */
  uint8_t   path_len;                    /* no. elements in path */
  uint8_t   path[USBH_MAX_EXT_HUBS + 1]; /* port path */
  uint8_t   speed;                       /* speed (USBH_..._SPEED) */
  uint8_t   rwkup;                       /* device on port supports remote wakeup */
  uint16_t  vid;                         /* vendor ID */
  uint16_t  pid;                         /* product ID */
  uint16_t  release;                     /* bcdDevice - device release number*/
}
t_usbh_port_inf;


/****************** EP info structure ********************/
typedef struct
{
  uint8_t   daddr;
  uint8_t   ep;
  uint8_t   type;
  uint8_t   dir;
  uint16_t  psize;
  uint8_t   interval;
} t_usbh_ep_inf;


/****************** HUB info structure ********************/
typedef struct
{
  uint8_t  dev_speed;
  uint8_t  hub_daddr;
  uint8_t  hub_port;
  uint8_t  hub_speed;
} t_usbh_ep_hub_inf;


/*********************** enumeration failed ************************/
typedef void ( * t_usbh_enum_failed_cb )( t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid );


/************************** config select **************************/
#define USBH_CONFIG_SELECT_ALL 0xff

typedef uint8_t ( * t_usbh_config_select_cb )( t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid );
typedef void ( * t_usbh_ep_info_change_cb )( t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid
                                               , t_usbh_ep_hub_inf * ep_hub_inf, t_usbh_ep_inf  * ep_inf );

/************************** notifications **************************/
#define USBH_NTF_CONNECT    1    /* connection notification code */
#define USBH_NTF_DISCONNECT 2    /* disconnection notification code */

#define USBH_NTF_CD_BASE    100  /* first notification the class driver can use */

typedef uint16_t  t_usbh_ntf;
typedef int   ( * t_usbh_ntf_fn )( t_usbh_unit_id uid, t_usbh_ntf ntf );

/*************************** test modes ****************************/
typedef enum
{
  USBH_TEST_MODE_OFF = 0
  , USBH_TEST_MODE_J_STATE
  , USBH_TEST_MODE_K_STATE
  , USBH_TEST_MODE_SE0_NAK
  , USBH_TEST_MODE_PACKET
  , USBH_TEST_MODE_FORCE_ENABLE
}
t_usbh_test_mode;


/*********************** Function prototypes ***********************/
int usbh_init ( void );
int usbh_start ( void );
int usbh_stop ( void );
int usbh_delete ( void );

int usbh_hc_init ( uint8_t id, void * hc, t_usbh_unit_id unit );
int usbh_hc_start ( uint8_t id );
int usbh_hc_stop ( uint8_t id );
int usbh_hc_delete ( uint8_t id );

void usbh_delay ( uint32_t ms );

int usbh_get_string ( t_usbh_port_hdl port_hdl, uint16_t idx, uint16_t lang_id, uint8_t * str, uint16_t mlen );
int usbh_get_port_inf ( t_usbh_port_hdl port_hdl, t_usbh_port_inf * p_port_inf );
int usbh_get_port_inf_port ( uint8_t id, uint8_t * p_path, uint8_t path_len, t_usbh_port_inf * p_port_inf );

int usbh_suspend ( t_usbh_port_hdl port_hdl, uint8_t rwkup_en );
int usbh_resume ( t_usbh_port_hdl port_hdl );
int usbh_test_mode_device ( t_usbh_port_hdl port_hdl, t_usbh_test_mode mode );
int usbh_test_mode_port ( uint8_t id, uint8_t * p_path, uint8_t path_len, t_usbh_test_mode mode );
int usbh_reenumerate ( t_usbh_port_hdl port_hdl );

int usbh_register_enum_failed_cb ( t_usbh_enum_failed_cb p_cb );
int usbh_register_config_select_cb ( t_usbh_config_select_cb p_cb );
int usbh_register_ep_info_change_cb ( t_usbh_ep_info_change_cb p_cb );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USB_HOST_H */

