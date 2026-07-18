/***************************************************************************
 *
 *            Copyright (c) 2008-2019 by HCC Embedded
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
#ifndef _CONFIG_USBD_MST_
#define _CONFIG_USBD_MST_

#include "config_oal_os.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../version/ver_usbd_mst.h"
#if VER_USBD_MST_MAJOR != 6 || VER_USBD_MST_MINOR != 16
 #error "Incorrect USBD_MST version."
#endif


/* The size of one RAM buffer. Must be longer than the sector size of the
   media. */
#define MST_BUFFER_SIZE       ( 1 * 1024u )

#define MST_OVERLAPPED        1

#define MST_EVENT_FLAG        1

#define MST_USE_MST_IF        1 /* use normal MST interface */

#define MST_USE_VMST_IF       0 /* use VMST interface */

#if ( MST_USE_MST_IF )
 #define USBD_MST_TASK_PRIO   OAL_NORMAL_PRIORITY

 #define USBD_MST_STACK_SIZE  1024
#endif

#if ( MST_USE_VMST_IF )
 #define USBD_VMST_TASK_PRIO  OAL_NORMAL_PRIORITY

 #define USBD_VMST_STACK_SIZE 1024
#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CONFIG_USBD_MST_ */
