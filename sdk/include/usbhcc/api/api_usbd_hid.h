/***************************************************************************
 *
 *            Copyright (c) 2009-2018 by HCC Embedded
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
#ifndef _API_USBD_HID_H_
#define _API_USBD_HID_H_

#include "../psp/include/psp_types.h"

#include "../version/ver_usbd_hid.h"
#if VER_USBD_HID_MAJOR != 8 || VER_USBD_HID_MINOR != 5
 #error "Incorrect HID class-driver version."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define USBD_HID_SUCCESS       0x00
#define USBD_HID_ERR_NOT_READY 0x01
#define USBD_HID_ERR_BUSY      0x02
#define USBD_HID_ERR_INIT      0x03
#define USBD_HID_ERR_RESOURCE  0x04
#define USBD_HID_ERR_REG_CDRV  0x05
#define USBD_HID_ERROR         0xFF

typedef uint16_t  t_usbd_hid_ntf;
typedef int   ( * t_usbd_hid_ntf_fn )( uint8_t unit, t_usbd_hid_ntf ntf, uint32_t param );

int usbd_hid_init ( void );
int usbd_hid_start ( void );
int usbd_hid_stop ( void );
int usbd_hid_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBD_HID_H_ */
