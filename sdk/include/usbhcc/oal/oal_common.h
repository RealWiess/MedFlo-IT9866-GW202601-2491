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
#ifndef OAL_COMMON_H
#define OAL_COMMON_H

#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR != 8
 #error Incompatible OAL version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* OAL error codes */
#define OAL_SUCCESS      0
#define OAL_ERR_RESOURCE 1
#define OAL_ERR_TIMEOUT  2
#define OAL_ERROR        3

/* 495 S : MISRA-C:2012/AMD1/TC1 D.4.6: Typedef name has no size indication. */
/*LDRA_INSPECTED 495 S*/
typedef int  oal_ret_t;

#ifdef __cplusplus
}
#endif

#endif /* ifndef OAL_COMMON_H */

