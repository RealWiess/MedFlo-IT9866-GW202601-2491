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
#ifndef CONFIG_OAL_OS_H
#define CONFIG_OAL_OS_H

/* OAL user definition file for FreeRTOS */
#include "../version/ver_oal_os.h"
#if VER_OAL_FREERTOS_MAJOR != 2 || VER_OAL_FREERTOS_MINOR != 9
 #error Incompatible OAL_FREERTOS version number!
#endif
#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR < 2
 #error Incompatible OAL version number!
#endif

/* priorities */
#define OAL_HIGHEST_PRIORITY 5
#define OAL_HIGH_PRIORITY    4
#define OAL_NORMAL_PRIORITY  3
#define OAL_LOW_PRIORITY     2
#define OAL_LOWEST_PRIORITY  1

/* Event flag to use for user tasks invoking internal functions. */
/* E.g.: One task calls an internal function that needs to wait for an event */
/* NOTE: The value of this flag should be > 0x80 because lower bits */
/* might be used by internal tasks */
#define OAL_EVENT_FLAG 0x100u

/* no. of max. interrupt sources */
#if (CFG_CHIP_FAMILY == 970) // Irene Lin
#define OAL_ISR_COUNT  3
#else
#define OAL_ISR_COUNT  2
#endif


/* Controls if architecture uses FPU. */
/* If it is  set to 1 every task has added a portTASK_USES_FLOATING_POINT() */
/* at start to prevent errors when compiler decides to use FPU registers */
/* in optimization process. */
#define OAL_USE_FPU 0

/* Set this to 1u, if the major version number of FreeRTOS is below 5. */
#define OAL_FREERTOS_MAJOR_VERSION_IS_BELOW_5 0u

/* Set this to 1u, if the target is AVR32. */
#define OAL_AVR32_PORT 0u

#include <FreeRTOS.h>
#if INCLUDE_vTaskSuspend == 0
 #error OAL - INCLUDE_vTaskSuspend must be set!
#endif
#if INCLUDE_xTaskGetCurrentTaskHandle == 0
 #error OAL - INCLUDE_xTaskGetCurrentTaskHandle must be set!
#endif
#if INCLUDE_vTaskDelay == 0
 #error OAL - INCLUDE_vTaskDelay must be set!
#endif
#if INCLUDE_vTaskDelete == 0
 #error OAL - INCLUDE_vTaskDelete must be set!
#endif
#if configUSE_MUTEXES == 0
 #error OAL - configUSE_MUTEXES must be set!
#endif

#endif /* ifndef CONFIG_OAL_OS_H */
