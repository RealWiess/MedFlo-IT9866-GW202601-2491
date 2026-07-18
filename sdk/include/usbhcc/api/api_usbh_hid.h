/***************************************************************************
 *
 *            Copyright (c) 2010-2023 by Tuxera
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

#ifndef _API_USBH_HID_H_
#define _API_USBH_HID_H_

#include "../psp/include/psp_types.h"
#include "../config/config_usbh_hid.h"

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 10
 #error Incompatible USBH_HID version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif


#define HID_DATA_TYPE_VARIABLE     0u
#define HID_DATA_TYPE_CONST        1u

#define USBH_HID_HDL_INVALID       NULL
typedef void * t_usbh_hid_hdl;

#define REPORT_ITEM_OFFSET_INVALID 0xFFu

#define USBH_HID_IS_ATTRIBUTE_CONST( i )         ( ( i ) & ( 1 << 0 ) )
#define USBH_HID_IS_ATTRIBUTE_DATA( i )          ( !USBH_HID_IS_ATTRIBUTE_CONST( i ) )
#define USBH_HID_IS_ATTRIBUTE_VARIABLE( i )      ( ( i ) & ( 1 << 1 ) )
#define USBH_HID_IS_ATTRIBUTE_ARRAY( i )         ( !USBH_HID_IS_ATTRIBUTE_VARIABLE( i ) )
#define USBH_HID_IS_ATTRIBUTE_RELATIVE( i )      ( ( i ) & ( 1 << 2 ) )
#define USBH_HID_IS_ATTRIBUTE_ABSOLUTE( i )      ( !USBH_HID_IS_ATTRIBUTE_RELATIVE( i ) )
#define USBH_HID_IS_ATTRIBUTE_WRAP( i )          ( ( i ) & ( 1 << 3 ) )
#define USBH_HID_IS_ATTRIBUTE_NOWRAP( i )        ( !USBH_HID_IS_ATTRIBUTE_WRAP( i ) )
#define USBH_HID_IS_ATTRIBUTE_NONLINEAR( i )     ( ( i ) & ( 1 << 4 ) )
#define USBH_HID_IS_ATTRIBUTE_LINEAR( i )        ( !USBH_HID_IS_ATTRIBUTE_NONLINEAR( i ) )
#define USBH_HID_IS_ATTRIBUTE_NOPREFST( i )      ( ( i ) & ( 1 << 5 ) )
#define USBH_HID_IS_ATTRIBUTE_PREFST( i )        ( !USBH_HID_IS_ATTRIBUTE_NOPREFST( i ) )
#define USBH_HID_IS_ATTRIBUTE_NONULLPOS( i )     ( ( i ) & ( 1 << 6 ) )
#define USBH_HID_IS_ATTRIBUTE_NULLPOS( i )       ( !USBH_HID_IS_ATTRIBUTE_NONULLPOS( i ) )
#define USBH_HID_IS_ATTRIBUTE_VOLATILE( i )      ( ( i ) & ( 1 << 7 ) )
#define USBH_HID_IS_ATTRIBUTE_NONVOLATILE( i )   ( !USBH_HID_IS_ATTRIBUTE_VOLATILE( i ) )
#define USBH_HID_IS_ATTRIBUTE_BUFFEREDBYTES( i ) ( ( i ) & ( 1 << 8 ) )
#define USBH_HID_IS_ATTRIBUTE_BITFIELD( i )      ( !USBH_HID_IS_ATTRIBUTE_BUFFEREDBYTES( i ) )
#define USBH_HID_IS_ATTRIBUTE_UNKNOWN( i )       ( ( i ) & ~( ( 1 << 9 ) - 1 ) )

#define USBH_HID_UNIT_SYSTEM_NONE               0
#define USBH_HID_UNIT_SYSTEM_SI_LINEAR          1
#define USBH_HID_UNIT_SYSTEM_SI_ROTATION        2
#define USBH_HID_UNIT_SYSTEM_ENGLISH_LINEAR     3
#define USBH_HID_UNIT_SYSTEM_ENGLISH_ROTATION   4

#define USBH_HID_UNIT_EXTENT_LENGTH             1
#define USBH_HID_UNIT_EXTENT_MASS               2
#define USBH_HID_UNIT_EXTENT_TIME               3
#define USBH_HID_UNIT_EXTENT_TEMPERATURE        4
#define USBH_HID_UNIT_EXTENT_CURRENT            5
#define USBH_HID_UNIT_EXTENT_LUMINOUS_INTENSITY 6


/* Report data item. */
typedef struct
{
  int32_t   logical_min;    /*min value device can return */
  int32_t   logical_max;    /*max value device can return */
  int32_t   physical_min;   /*min vale read can report */
  int32_t   physical_max;   /*max value read can report */
  int32_t   unit_exponent;
  uint32_t  unit;
  uint32_t  resolution;     /* ratio between logical and physical value */
  uint16_t  usage_page;
  uint16_t  usage_min;
  uint16_t  usage_max;      /* only used for arrays */
  uint8_t   ary_size;
  uint8_t   offset;         /* offset of first byte in buffer */
  uint8_t   shift;          /* offset of firs bit in LSB */
  uint8_t   size;           /* size in bits */
  uint8_t   sign;           /* 1 if item is signed. */
  uint8_t   data_type;      /* HID_DATA_TYPE_CONST if item is constant. */
  uint32_t  attribute;      /* Input, Output, and Feature data fields. */
} t_report_item;

/* Possible types for a report. */
typedef enum
{
  rpt_invalid
  , rpt_in
  , rpt_out
  , rpt_feature
} t_report_type;

typedef struct
{
  t_report_type     type;
  uint8_t           id;
  uint16_t          bit_ofs;
  uint16_t          total_item_count;
  uint16_t          valid_item_count;
  uint16_t          report_size;
  t_report_item * * items;
  uint8_t           usage;
} t_report;


int usbh_hid_init ( void );
int usbh_hid_start ( void );
int usbh_hid_stop ( void );
int usbh_hid_delete ( void );

int usbh_hid_get_report_descriptor ( t_usbh_hid_hdl hid_hdl, uint8_t * buffer, uint16_t length );
int usbh_hid_set_protocol ( t_usbh_hid_hdl hid_hdl, uint8_t boot );
int usbh_hid_set_idle ( t_usbh_hid_hdl hid_hdl, uint8_t report_id, uint8_t dur );

int usbh_hid_set_report ( t_usbh_hid_hdl hid_hdl, uint8_t report_id, uint8_t * buffer, uint16_t length );
int usbh_hid_get_report ( t_usbh_hid_hdl hid_hdl, uint8_t report_id, uint8_t * buffer, uint16_t length );
int usbh_hid_set_feature_report ( t_usbh_hid_hdl hid_hdl, uint8_t report_id, uint8_t * buffer, uint16_t length );
int usbh_hid_get_feature_report ( t_usbh_hid_hdl hid_hdl, uint8_t report_id, uint8_t * buffer, uint16_t length );

int usbh_hid_unit_get_system ( uint32_t unit, uint8_t * system );
int usbh_hid_unit_get_exponent ( uint32_t unit, uint8_t extent, int8_t * exponent );


int usbh_hid_read_item ( t_report_item * ri, uint8_t * buffer, uint8_t index, int32_t * value );
int usbh_hid_write_item ( t_report_item * ri, uint8_t * buffer, uint8_t index, int32_t value );


#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_HID_H_ */
