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
#ifndef OALP_EVENT_H
#define OALP_EVENT_H

#include <stdint.h>

#include "oalp_mutex.h"

#include <FreeRTOS.h>
#include <queue.h>

#include "../../version/ver_oal_os.h"
#if VER_OAL_FREERTOS_MAJOR != 2 || VER_OAL_FREERTOS_MINOR != 9
 #error Incompatible OAL_FREERTOS version number!
#endif
#include "../../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR < 2
 #error Incompatible OAL version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  oal_mutex_t   mutex;
  xQueueHandle  event;
} oal_event_t;

/* 495 S : MISRA-C:2004 6.3: Typedef name has no size indication. */
/*LDRA_INSPECTED 495 S*/
typedef uint16_t  oal_event_flags_t;

/*LDRA_INSPECTED 495 S*/
typedef portTickType  oal_event_timeout_t;

#define OAL_WAIT_FOREVER portMAX_DELAY

#ifdef __cplusplus
}
#endif

#endif /* ifndef OALP_EVENT_H */

