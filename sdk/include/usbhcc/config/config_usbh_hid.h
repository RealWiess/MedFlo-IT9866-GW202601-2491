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
#ifndef _CONFIG_USBH_HID_H
#define _CONFIG_USBH_HID_H

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 10
 #error Incompatible USBH_HID version number!
#endif

/* set to enable mouse support */
#define HID_MOUSE_ENABLE                         1u

/* set to enable keyboard support */
#define HID_KBD_ENABLE                           0u//1u

/* set to enable generic support */
#define HID_GENERIC_ENABLE                       1u

/* set to enable joystick/gamepad support */
#define HID_JOYSTICK_ENABLE                      0u//1u

/* maximum size of report descriptor */
#define HID_REPORT_DESCRIPTOR_MAX_SIZE           1024u//512u


/* must be 1 or higher: during parsing, a report descriptor
needs temporal storage for the global properties.
The depth of the storage must be at least the maximum count
of consecutive push instructions within a report descriptor
plus one */
#define HID_PARSER_GLOBAL_PROPERTIES_STACK_DEPTH 1u

/* the maximum number of reports within a report descriptor */
#define HID_PARSER_MAXIMUM_REPORT_COUNT          24u//4u

/* the maximum number of report items within any report that the parser can process */
#define HID_PARSER_MAXIMUM_REPORT_ITEM_COUNT     64u

/* Zero value means that the HID calculates the pool size. */
#define HID_ITEM_POOL_SIZE                       0u

/* Report buffer size */
#define HID_REPORT_DATA_SIZE                     128u

#endif /* ifndef _CONFIG_USBH_HID_H */

