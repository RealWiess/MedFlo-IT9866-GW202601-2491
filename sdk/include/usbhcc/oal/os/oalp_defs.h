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
#ifndef OALP_DEFS_H
#define OALP_DEFS_H

#include "../../config/config_oal_os.h"
#include <FreeRTOS.h>

#include "../../version/ver_oal_os.h"
#if VER_OAL_FREERTOS_MAJOR != 2 || VER_OAL_FREERTOS_MINOR != 9
 #error Incompatible OAL_FREERTOS version number!
#endif
#include "../../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR < 2
 #error Incompatible OAL version number!
#endif

/* tasks are polled - only set with no OS */
#define OAL_TASK_POLL_MODE    0

/* preemptive system */
#define OAL_PREEMPTIVE        1

/* stack of a task needs to be allocated statically */
#define OAL_STATIC_TASK_STACK 0

/* allow interrupts - can only be unset if no OS */
#define OAL_INTERRUPT_ENABLE  1

/* use platform ISR routines */
#define OAL_USE_PLATFORM_ISR  1

/* tick rate in ms */
#define OAL_TICK_RATE         portTICK_RATE_MS

#endif /* ifndef OALP_DEFS_H */

