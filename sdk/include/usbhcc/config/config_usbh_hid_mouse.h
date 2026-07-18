/***************************************************************************
 *
 *            Copyright (c) 2011-2023 by Tuxera
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
#ifndef _CONFIG_USBH_HID_MOUSE_H
#define _CONFIG_USBH_HID_MOUSE_H

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 10
 #error Incompatible USBH_HID version number!
#endif

#define MAX_MOUSE_UNITS   1
#define MAX_MOUSE_BUTTONS 16

#endif

